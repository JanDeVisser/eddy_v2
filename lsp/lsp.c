/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdarg.h>
#include <unistd.h>

#include <app/eddy.h>
#include <app/theme.h>
#include <lsp/lsp.h>
#include <lsp/schema/InitializeParams.h>
#include <lsp/schema/InitializeResult.h>

DA_IMPL(LSP);
DA_IMPL(Request);

static void lsp_initialize_theme_internal(LSP *lsp);

LSP the_lsp = {0};

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
    // trace(LSP, "response_decode():\n%.*s\n", SV_ARG(json_encode(*json)));
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
    // trace(LSP, "response_decode():\n%.*s\n", SV_ARG(json_encode(*json)));
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
    if (!log_category_on(LSP)) {
        return;
    }
    va_list args;
    va_start(args, msg);
    vtrace(__FILE_NAME__, __LINE__, (TraceCategory) {"LSP", 3 }, msg, args);
    if (!json.has_value) {
        trace(LSP, "(no json)");
        return;
    }
    va_end(args);
    StringView s = json_to_string(json.value);
    trace(LSP, "%.*s", SV_ARG(s));
    sv_free(s);
}

ErrorOrInt lsp_message(LSP *lsp, void *sender, char const *method, OptionalJSONValue params)
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
    for (int ix = 0; ix < lsp->request_queue.size; ++ix) {
        if (sv_empty(req.method)) {
            req_ix = ix;
            lsp->request_queue.elements[ix] = req;
            break;
        }
    }
    if (req_ix < 0) {
        da_append_Request(&lsp->request_queue, req);
    }
    assert(lsp->lsp);
    JSONValue json = request_encode(&req);
    trace(LSP, "==> %s (%d)", method, req.id);
    StringView request_json = json_encode(json);
    StringView req_content_length = sv_printf("Content-Length: %zu\r\n\r\n", request_json.length + 2);
    TRY_TO(Size, Int, write_pipe_write(&lsp->lsp->in, req_content_length));
    sv_free(req_content_length);
    TRY_TO(Size, Int, write_pipe_write(&lsp->lsp->in, request_json));
    TRY_TO(Size, Int, write_pipe_write(&lsp->lsp->in, sv_from("\r\n")));
    sv_free(request_json);
    RETURN(Int, 0);
}

void handle_initialize_response(LSP *lsp, Widget *, JSONValue response_json)
{
    condition_acquire(lsp->init_condition);
    Response response = response_decode(&response_json);
    if (response_success(&response)) {
        InitializeResult result = MUST_OPTIONAL(InitializeResult, InitializeResult_decode(response.result));
        if (result.serverInfo.has_value) {
            trace(LSP, "LSP server name: %.*s", SV_ARG(result.serverInfo.name));
            if (result.serverInfo.version.has_value) {
                trace(LSP, "LSP server version: %.*s", SV_ARG(result.serverInfo.version.value));
            }
        }
        lsp->server_capabilities = result.capabilities;
        assert(lsp->server_capabilities.semanticTokensProvider.has_value);
        // trace(LSP, "#Token types: %zu", server_capabilities.semanticTokensProvider.value.legend.tokenTypes.size);
        lsp_initialize_theme_internal(lsp);
    }
    MUST(Int, lsp_notification(lsp, "initialized", OptionalJSONValue_create(json_object())));
    lsp->lsp_ready = true;
    condition_broadcast(lsp->init_condition);
}

void lsp_read(ReadPipe *pipe)
{
    LSP *lsp = (LSP *) pipe->context;
    trace(LSP, "lsp_read");
    do {
        sb_append_sv((StringBuilder *) &lsp->lsp_scanner.string, read_pipe_current(pipe));
        do {
            if (!ss_expect_sv(&lsp->lsp_scanner, SV("Content-Length:", 15))) {
                trace(LSP, "No content-length header");
                return;
            }
            ss_skip_whitespace(&lsp->lsp_scanner);
            size_t resp_content_length = ss_read_number(&lsp->lsp_scanner);
            if (!resp_content_length) {
                trace(LSP, "No content-length value");
                ss_rewind(&lsp->lsp_scanner);
                return;
            }
            if (!ss_expect_sv(&lsp->lsp_scanner, SV("\r\n\r\n", 4))) {
                trace(LSP, "No header-content separator");
                ss_rewind(&lsp->lsp_scanner);
                return;
            }
            StringView response_json = ss_read(&lsp->lsp_scanner, resp_content_length);
            if (response_json.length < resp_content_length) {
                trace(LSP, "Short content");
                ss_rewind(&lsp->lsp_scanner);
                return;
            }
            ErrorOrJSONValue ret_maybe = json_decode(response_json);
            if (ErrorOrJSONValue_is_error(ret_maybe)) {
                info("ERROR Parsing incoming JSON: %s", Error_to_string(ret_maybe.error));
                trace(LSP, "****** <== %.*s", SV_ARG(response_json));
                eddy_set_message(&eddy, "LSP: %s", Error_to_string(ret_maybe.error));
                goto defer_0;
            }
            // trace_json(OptionalJSONValue_create(ret_maybe.value), "<==");
            JSONValue ret = ret_maybe.value;
            if (json_has(&ret, "id")) {
                Response response = response_decode(&ret);
                trace(LSP, "<== %d", response.id);
                for (size_t ix = 0; ix < lsp->request_queue.size; ++ix) {
                    Request *req = lsp->request_queue.elements + ix;
                    if (req->id == response.id) {
                        if (sv_eq_cstr(req->method, "initialize")) {
                            trace(LSP, "<== ** initialize **");
                            handle_initialize_response(lsp, req->sender, ret);
                            goto defer_0;
                        }
                        trace(LSP, "<== %.*s", SV_ARG(req->method));
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
            trace(LSP, "|== %.*s`", SV_ARG(notification.method));
            StringView cmd = sv_printf("lsp-%.*s", SV_ARG(notification.method));
            app_submit(app, app, cmd, ret);
            sv_free(cmd);
            notification_free(&notification);
        defer_0:
            ss_reset(&lsp->lsp_scanner);
        } while (lsp->lsp_scanner.point.index < lsp->lsp_scanner.string.length);
        lsp->lsp_scanner.point = (TextPosition) { 0 };
        lsp->lsp_scanner.string.length = 0;
        ss_reset(&lsp->lsp_scanner);
    } while (pipe->current.length > 0);
}

ErrorOrInt lsp_notification(LSP *lsp, char const *method, OptionalJSONValue params)
{
    Notification notification = { sv_from(method), OptionalJSONValue_empty() };
    notification.params = params;
    assert(lsp->lsp);
    StringView  json = json_encode(notification_encode(&notification));
    char const *content_length = TextFormat("Content-Length: %zu\r\n\r\n", json.length + 2);
    TRY_TO(Size, Int, write_pipe_write(&lsp->lsp->in, sv_from(content_length)));
    TRY_TO(Size, Int, write_pipe_write(&lsp->lsp->in, json));
    TRY_TO(Size, Int, write_pipe_write(&lsp->lsp->in, sv_from("\r\n")));
    trace(LSP, "==| %s", method);
    RETURN(Int, 0);
}

void lsp_initialize_theme_internal(LSP *lsp)
{
    for (int i = 0; i < lsp->server_capabilities.semanticTokensProvider.value.legend.tokenTypes.size; ++i) {
        StringView                 tokenType = lsp->server_capabilities.semanticTokensProvider.value.legend.tokenTypes.strings[i];
        OptionalSemanticTokenTypes semantic_token_type_maybe = SemanticTokenTypes_parse(tokenType);
        if (!semantic_token_type_maybe.has_value) {
            trace(LSP, "SemanticTokenType %d = '%.*s' not recognized", i, SV_ARG(tokenType));
            continue;
        }
        SemanticTokenTypes type = semantic_token_type_maybe.value;
        theme_map_semantic_type(&eddy.theme, i, type);
    }
}

void lsp_initialize_theme(LSP *lsp)
{
    if (!lsp->lsp_ready) {
        return;
    }
    lsp_initialize_theme_internal(lsp);
}

void lsp_initialize(LSP *lsp)
{
    lsp->init_condition = condition_create();
    if (lsp->lsp_ready) {
        return;
    }
    condition_acquire(lsp->init_condition);
    if (lsp->lsp_ready) {
        condition_release(lsp->init_condition);
        assert(lsp->lsp_ready);
        return;
    }
    if (lsp->lsp != NULL) {
        condition_sleep(lsp->init_condition);
        assert(lsp->lsp_ready);
        return;
    }
    trace(LSP, "Initializing LSP");
    lsp->lsp = lsp->handlers.start(lsp);
    lsp->lsp->out.on_read = lsp_read;
    lsp->lsp->out.context = &lsp;
    process_background(lsp->lsp);

    InitializeParams params = { 0 };
    params.processId.has_value = true;
    params.processId.tag = 0;
    params.processId._0 = getpid();
    params.clientInfo.has_value = true;
    params.clientInfo.name = SV(EDDY_NAME);
    params.clientInfo.version = (OptionalStringView) { .has_value = true, .value = SV(EDDY_VERSION) };
    params.rootUri.has_value = true;
    params.rootUri.tag = 0;
    params.rootUri._0 = sv_printf("file://%.*s", SV_ARG(eddy.project_dir));
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
    MUST(Int, lsp_message(lsp, &eddy, "initialize", params_json));
    condition_sleep(lsp->init_condition);
}
