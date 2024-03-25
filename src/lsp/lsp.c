/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdarg.h>
#include <sys/syslimits.h>
#include <unistd.h>

#include <app/eddy.h>
#include <app/palette.h>
#include <base/json.h>
#include <base/process.h>
#include <base/threadonce.h>
#include <lsp/lsp.h>
#include <lsp/schema/InitializeParams.h>
#include <lsp/schema/InitializeResult.h>
#include <lsp/schema/SemanticTokenTypes.h>
#include <lsp/schema/ServerCapabilities.h>

DA_WITH_NAME(Request, Requests);
DA_IMPL(Request);

PaletteIndex semantic_token_colors[30];

THREAD_ONCE(lsp_init);
static Condition          init_condition;
static Process           *lsp = NULL;
static bool               lsp_ready = false;
static ServerCapabilities server_capabilties = { 0 };
static Requests           request_queue = { 0 };
static StringScanner      lsp_scanner = { 0 };

JSONValue notification_encode(Notification *notification)
{
    JSONValue ret = json_object();
    json_set_cstr(&ret, "jsonrpc", "2.0");
    json_set_string(&ret, "method", notification->method);
    if (notification->params.has_value) {
        json_set(&ret, "params", notification->params.value);
    }
    return ret;
}

Notification notification_decode(JSONValue *json)
{
    Notification ret = { 0 };
    // trace(CAT_LSP, "response_decode():\n%.*s\n", SV_ARG(json_encode(*json)));
    JSONValue mth = MUST_OPTIONAL(JSONValue, json_get(json, "method"));
    assert(mth.type == JSON_TYPE_STRING);
    ret.method = sv_copy(mth.string);
    OptionalJSONValue params = json_get(json, "params");
    if (params.has_value) {
        ret.params = OptionalJSONValue_create(json_copy(params.value));
    }
    return ret;
}

void notification_free(Notification *notification)
{
    sv_free(notification->method);
    notification->method = sv_null();
    if (notification->params.has_value) {
        json_free(notification->params.value);
        notification->params = (OptionalJSONValue) { 0 };
    }
}

JSONValue request_encode(Request *request)
{
    JSONValue ret = json_object();
    json_set_cstr(&ret, "jsonrpc", "2.0");
    json_set_int(&ret, "id", request->id);
    json_set_string(&ret, "method", request->method);
    if (request->params.has_value) {
        json_set(&ret, "params", request->params.value);
    }
    return ret;
}

void request_free(Request *request)
{
    sv_free(request->method);
    request->method = sv_null();
    if (request->params.has_value) {
        json_free(request->params.value);
        request->params = (OptionalJSONValue) { 0 };
    }
}

bool response_success(Response *response)
{
    return response->result.has_value;
}

bool response_error(Response *response)
{
    return response->error.has_value;
}

Response response_decode(JSONValue *json)
{
    Response ret = { 0 };
    // trace(CAT_LSP, "response_decode():\n%.*s\n", SV_ARG(json_encode(*json)));
    JSONValue id = MUST_OPTIONAL(JSONValue, json_get(json, "id"));
    assert(id.type == JSON_TYPE_INT);
    ret.id = json_int_value(id);
    OptionalJSONValue result = json_get(json, "result");
    if (result.has_value) {
        ret.result = OptionalJSONValue_create(json_copy(result.value));
    }
    OptionalJSONValue error = json_get(json, "error");
    if (error.has_value) {
        ret.error = OptionalJSONValue_create(json_copy(error.value));
    }
    assert(response_success(&ret) ^ response_error(&ret));
    return ret;
}

void trace_json(OptionalJSONValue json, char const *msg, ...)
{
    if (!log_category_on(CAT_LSP)) {
        return;
    }
    va_list args;
    va_start(args, msg);
    vtrace(CAT_LSP, msg, args);
    if (!json.has_value) {
        trace(CAT_LSP, "(no json)");
        return;
    }
    va_end(args);
    StringView s = json_to_string(json.value);
    trace(CAT_LSP, "%.*s", SV_ARG(s));
    sv_free(s);
}

ErrorOrInt lsp_message(void *sender, char const *method, OptionalJSONValue params)
{
    static int id = 1;
    Request    req = {
           .sender = sender,
           .id = id++,
           .method = sv_from(method),
           .params = params,
    };
    req.params = params;
    int req_ix = -1;
    for (int ix = 0; ix < request_queue.size; ++ix) {
        Request *r = request_queue.elements + ix;
        if (sv_empty(req.method)) {
            req_ix = ix;
            request_queue.elements[ix] = req;
            break;
        }
    }
    if (req_ix < 0) {
        da_append_Request(&request_queue, req);
    }
    assert(lsp);
    JSONValue json = request_encode(&req);
    trace(CAT_LSP, "==> %s (%d)", method, req.id);
    StringView request_json = json_encode(json);
    StringView req_content_length = sv_printf("Content-Length: %zu\r\n\r\n", request_json.length + 2);
    TRY_TO(Size, Int, write_pipe_write(&lsp->in, req_content_length));
    sv_free(req_content_length);
    TRY_TO(Size, Int, write_pipe_write(&lsp->in, request_json));
    TRY_TO(Size, Int, write_pipe_write(&lsp->in, sv_from("\r\n")));
    sv_free(request_json);
    RETURN(Int, 0);
}

void handle_initialize_response(Widget *app, JSONValue response_json);

void lsp_read(ReadPipe *pipe)
{
    trace(CAT_LSP, "lsp_read");
    sb_append_sv((StringBuilder *) &lsp_scanner.string, read_pipe_current(pipe));
    if (!ss_expect_sv(&lsp_scanner, SV("Content-Length:", 15))) {
        trace(CAT_LSP, "No content-length header");
        return;
    }
    ss_skip_whitespace(&lsp_scanner);
    size_t resp_content_length = ss_read_number(&lsp_scanner);
    if (!resp_content_length) {
        trace(CAT_LSP, "No content-length value");
        ss_rewind(&lsp_scanner);
        return;
    }
    if (!ss_expect_sv(&lsp_scanner, SV("\r\n\r\n", 4))) {
        trace(CAT_LSP, "No header-content separator");
        ss_rewind(&lsp_scanner);
        return;
    }
    StringView response_json = ss_read(&lsp_scanner, resp_content_length);
    if (response_json.length < resp_content_length) {
        trace(CAT_LSP, "Short content");
        ss_rewind(&lsp_scanner);
        return;
    }
    ErrorOrJSONValue ret_maybe = json_decode(response_json);
    if (ErrorOrJSONValue_is_error(ret_maybe)) {
        info("ERROR Parsing incoming JSON: %s", Error_to_string(ret_maybe.error));
        trace(CAT_LSP, "****** <== %.*s", SV_ARG(response_json));
        eddy_set_message(&eddy, "LSP: %s", Error_to_string(ret_maybe.error));
        goto defer_0;
    }
    // trace_json(OptionalJSONValue_create(ret_maybe.value), "<==");
    JSONValue ret = ret_maybe.value;
    if (json_has(&ret, "id")) {
        Response response = response_decode(&ret);
        trace(CAT_LSP, "<== %d", response.id);
        for (size_t ix = 0; ix < request_queue.size; ++ix) {
            Request *req = request_queue.elements + ix;
            if (req->id == response.id) {
                if (sv_eq_cstr(req->method, "initialize")) {
                    trace(CAT_LSP, "<== ** initialize **");
                    handle_initialize_response(req->sender, ret);
                    goto defer_0;
                }
                trace(CAT_LSP, "<== %.*s", SV_ARG(req->method));
                StringView cmd = sv_printf("lsp-%.*s", SV_ARG(req->method));
                app_submit(app, req->sender, cmd, ret);
                sv_free(cmd);
                goto defer_0;
            }
        }
        info("No request matching LSP response ID %d found", response.id);
        goto defer_0;
    }
    assert(json_has(&ret, "method"));
    Notification notification = notification_decode(&ret);
    trace(CAT_LSP, "|== %.*s", SV_ARG(notification.method));
    StringView cmd = sv_printf("lsp-%.*s", SV_ARG(notification.method));
    app_submit(app, app, cmd, ret);
    sv_free(cmd);
    notification_free(&notification);
defer_0:
    if (lsp_scanner.point.index == lsp_scanner.string.length) {
        lsp_scanner.point = (TextPosition) { 0 };
        lsp_scanner.string.length = 0;
    }
    ss_reset(&lsp_scanner);
}

ErrorOrInt lsp_notification(char const *method, OptionalJSONValue params)
{
    Notification notification = { sv_from(method), OptionalJSONValue_empty() };
    notification.params = params;
    assert(lsp);
    StringView  json = json_encode(notification_encode(&notification));
    char const *content_length = TextFormat("Content-Length: %zu\r\n\r\n", json.length + 2);
    TRY_TO(Size, Int, write_pipe_write(&lsp->in, sv_from(content_length)));
    TRY_TO(Size, Int, write_pipe_write(&lsp->in, json));
    TRY_TO(Size, Int, write_pipe_write(&lsp->in, sv_from("\r\n")));
    trace(CAT_LSP, "==| %s", method);
    RETURN(Int, 0);
}

void lsp_initializer_fnc()
{
    init_condition = condition_create();
}

void handle_initialize_response(Widget *app, JSONValue response_json)
{
    condition_acquire(init_condition);
    Response response = response_decode(&response_json);
    if (response_success(&response)) {
        InitializeResult result = MUST_OPTIONAL(InitializeResult, InitializeResult_decode(response.result));
        if (result.serverInfo.has_value) {
            trace(CAT_LSP, "LSP server name: %.*s", SV_ARG(result.serverInfo.name));
            if (result.serverInfo.version.has_value) {
                trace(CAT_LSP, "LSP server version: %.*s", SV_ARG(result.serverInfo.version.value));
            }
        }
        server_capabilties = result.capabilities;
        assert(server_capabilties.semanticTokensProvider.has_value);
        // trace(CAT_LSP, "#Token types: %zu", server_capabilties.semanticTokensProvider.value.legend.tokenTypes.size);
        for (int i = 0; i < server_capabilties.semanticTokensProvider.value.legend.tokenTypes.size; ++i) {
            StringView                 tokenType = server_capabilties.semanticTokensProvider.value.legend.tokenTypes.strings[i];
            OptionalSemanticTokenTypes semantic_token_type_maybe = SemanticTokenTypes_parse(tokenType);
            if (semantic_token_type_maybe.has_value) {
                // trace(CAT_LSP, "%d: '%.*s' %d", i, SV_ARG(tokenType), semantic_token_type);
                switch (semantic_token_type_maybe.value) {
                case SemanticTokenTypesNamespace:
                    semantic_token_colors[i] = PI_KNOWN_IDENTIFIER;
                    break;
                case SemanticTokenTypesType:
                    semantic_token_colors[i] = PI_KNOWN_IDENTIFIER;
                    break;
                case SemanticTokenTypesClass:
                    semantic_token_colors[i] = PI_KNOWN_IDENTIFIER;
                    break;
                case SemanticTokenTypesEnum:
                    semantic_token_colors[i] = PI_KNOWN_IDENTIFIER;
                    break;
                case SemanticTokenTypesInterface:
                    semantic_token_colors[i] = PI_KNOWN_IDENTIFIER;
                    break;
                case SemanticTokenTypesStruct:
                    semantic_token_colors[i] = PI_KNOWN_IDENTIFIER;
                    break;
                case SemanticTokenTypesTypeParameter:
                    semantic_token_colors[i] = PI_KNOWN_IDENTIFIER;
                    break;
                case SemanticTokenTypesParameter:
                    semantic_token_colors[i] = PI_KNOWN_IDENTIFIER;
                    break;
                case SemanticTokenTypesVariable:
                    semantic_token_colors[i] = PI_KNOWN_IDENTIFIER;
                    break;
                case SemanticTokenTypesProperty:
                    semantic_token_colors[i] = PI_KNOWN_IDENTIFIER;
                    break;
                case SemanticTokenTypesEnumMember:
                    semantic_token_colors[i] = PI_KNOWN_IDENTIFIER;
                    break;
                case SemanticTokenTypesEvent:
                    semantic_token_colors[i] = PI_DEFAULT;
                    break;
                case SemanticTokenTypesFunction:
                    semantic_token_colors[i] = PI_FUNCTION;
                    break;
                case SemanticTokenTypesMethod:
                    semantic_token_colors[i] = PI_KNOWN_IDENTIFIER;
                    break;
                case SemanticTokenTypesMacro:
                    semantic_token_colors[i] = PI_KNOWN_IDENTIFIER;
                    break;
                case SemanticTokenTypesKeyword:
                    semantic_token_colors[i] = PI_KEYWORD;
                    break;
                case SemanticTokenTypesModifier:
                    semantic_token_colors[i] = PI_KEYWORD;
                    break;
                case SemanticTokenTypesComment:
                    semantic_token_colors[i] = PI_COMMENT;
                    break;
                case SemanticTokenTypesString:
                    semantic_token_colors[i] = PI_STRING;
                    break;
                case SemanticTokenTypesNumber:
                    semantic_token_colors[i] = PI_NUMBER;
                    break;
                case SemanticTokenTypesRegexp:
                    semantic_token_colors[i] = PI_STRING;
                    break;
                case SemanticTokenTypesOperator:
                    semantic_token_colors[i] = PI_DEFAULT;
                    break;
                case SemanticTokenTypesDecorator:
                    semantic_token_colors[i] = PI_KNOWN_IDENTIFIER;
                    break;
                default:
                    semantic_token_colors[i] = PI_STRING;
                }
            }
        }
    }
    MUST(Int, lsp_notification("initialized", OptionalJSONValue_create(json_object())));
    lsp_ready = true;
    condition_broadcast(init_condition);
}

void lsp_initialize()
{
    ONCE(lsp_init, lsp_initializer_fnc);
    if (lsp_ready) {
        return;
    }
    condition_acquire(init_condition);
    if (lsp_ready) {
        condition_release(init_condition);
        assert(lsp_ready);
        return;
    }
    if (lsp != NULL) {
        condition_sleep(init_condition);
        assert(lsp_ready);
        return;
    }
    widget_register(app, "lsp-initialize", handle_initialize_response);
    lsp = process_create(SV("clangd", 6), "--use-dirty-headers", "--background-index");
    lsp->stderr_file = SV("/tmp/clangd.log", 15);
    lsp->out.on_read = lsp_read;
    process_background(lsp);

    InitializeParams params = { 0 };
    params.processId.has_value = true;
    params.processId.tag = 0;
    params.processId._0 = getpid();
    params.clientInfo.has_value = true;
    params.clientInfo.name = sv_from(EDDY_NAME);
    params.clientInfo.version = (OptionalStringView) { .has_value = true, .value = sv_from(EDDY_VERSION) };
    char prj[PATH_MAX + 8];
    memset(prj, '\0', PATH_MAX + 8);
    snprintf(prj, PATH_MAX + 7, "file://%.*s", SV_ARG(eddy.project_dir));
    params.rootUri.has_value = true;
    params.rootUri.tag = 0;
    params.rootUri._0 = sv_from(prj);
    params.capabilities.textDocument.has_value = true;

    StringList tokenTypes = { 0 };
    da_append_StringView(&tokenTypes, SemanticTokenTypes_to_string(SemanticTokenTypesComment));
    da_append_StringView(&tokenTypes, SemanticTokenTypes_to_string(SemanticTokenTypesKeyword));
    da_append_StringView(&tokenTypes, SemanticTokenTypes_to_string(SemanticTokenTypesVariable));
    da_append_StringView(&tokenTypes, SemanticTokenTypes_to_string(SemanticTokenTypesType));

    params.capabilities.textDocument.value.synchronization.has_value = true;
    params.capabilities.textDocument.value.synchronization.value.didSave.has_value = true;
    params.capabilities.textDocument.value.synchronization.value.didSave.value = true;
    params.capabilities.textDocument.value.synchronization.value.willSave.has_value = true;
    params.capabilities.textDocument.value.synchronization.value.didSave.value = false;
    params.capabilities.textDocument.value.synchronization.value.willSaveWaitUntil.has_value = true;
    params.capabilities.textDocument.value.synchronization.value.willSaveWaitUntil.value = false;
    params.capabilities.textDocument.value.semanticTokens.has_value = true;
    params.capabilities.textDocument.value.semanticTokens.value.multilineTokenSupport = (OptionalBool) { .has_value = true, .value = true };
    params.capabilities.textDocument.value.semanticTokens.value.requests.has_value = true;
    params.capabilities.textDocument.value.semanticTokens.value.requests.full.has_value = true;
    params.capabilities.textDocument.value.semanticTokens.value.requests.full._0 = true;
    params.capabilities.textDocument.value.semanticTokens.value.tokenTypes = tokenTypes;

    OptionalJSONValue params_json = InitializeParams_encode(params);
    MUST(Int, lsp_message(&eddy, "initialize", params_json));
    condition_sleep(init_condition);
}
