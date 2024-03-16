/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdarg.h>
#include <sys/syslimits.h>
#include <unistd.h>

#include <eddy.h>
#include <json.h>
#include <lsp/schema/DidChangeTextDocumentParams.h>
#include <lsp/schema/DidCloseTextDocumentParams.h>
#include <lsp/schema/DidOpenTextDocumentParams.h>
#include <lsp/schema/DidSaveTextDocumentParams.h>
#include <lsp/schema/DocumentFormattingParams.h>
#include <lsp/schema/InitializeParams.h>
#include <lsp/schema/InitializeResult.h>
#include <lsp/schema/SemanticTokenTypes.h>
#include <lsp/schema/SemanticTokens.h>
#include <lsp/schema/SemanticTokensParams.h>
#include <lsp/schema/ServerCapabilities.h>
#include <lsp/schema/TextEdit.h>
#include <lsp/schema/lsp_base.h>
#include <palette.h>
#include <process.h>

static Process           *lsp = NULL;
static ServerCapabilities server_capabilties;
static PaletteIndex       colors[30];

void trace_json(OptionalJSONValue json, char const *msg, ...)
{
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

ErrorOrResponse lsp_message(char const *method, OptionalJSONValue params)
{
    static int id = 0;
    Request    req = { .id = ++id, method, OptionalJSONValue_empty() };
    req.params = params;
    assert(lsp);
    JSONValue json = request_encode(&req);
    trace_json(OptionalJSONValue_create(json), "==>");
    StringView request_json = json_encode(json);
    StringView req_content_length = sv_printf("Content-Length: %zu\r\n\r\n", request_json.length + 2);
    TRY_TO(Size, Response, write_pipe_write(&lsp->in, req_content_length));
    sv_free(req_content_length);
    TRY_TO(Size, Response, write_pipe_write(&lsp->in, request_json));
    TRY_TO(Size, Response, write_pipe_write(&lsp->in, sv_from("\r\n")));

    StringView buf = { 0 };
    while (true) {
        read_pipe_expect(&lsp->out);
        StringView out = read_pipe_current(&lsp->out);
        if (buf.ptr == NULL) {
            buf = out;
        } else {
            assert(out.ptr == buf.ptr + buf.length);
            buf.length += out.length;
        }
        StringScanner ss = ss_create(buf);
        if (!ss_expect_sv(&ss, sv_from("Content-Length:"))) {
            continue;
        }
        ss_skip_whitespace(&ss);
        size_t resp_content_length = ss_read_number(&ss);
        if (!resp_content_length) {
            continue;
        }
        if (!ss_expect_sv(&ss, sv_from("\r\n\r\n"))) {
            continue;
        }
        StringView response_json = ss_read(&ss, resp_content_length);
        if (response_json.length < resp_content_length) {
            continue;
        }
        buf = (StringView) { 0 };
        ErrorOrJSONValue ret_maybe = json_decode(response_json);
        if (ErrorOrJSONValue_is_error(ret_maybe)) {
            trace(CAT_LSP, "==> %.*s", SV_ARG(request_json));
            trace(CAT_LSP, "****** <== %.*s", SV_ARG(response_json));
            return ErrorOrResponse_copy(ret_maybe.error);
        }
        trace_json(OptionalJSONValue_create(ret_maybe.value), "<==");
        JSONValue ret = ret_maybe.value;

        if (json_has(&ret, "method")) {
            // Notification. Drop on the floor
            trace(CAT_LSP, "Received '%.*s' notification", SV_ARG(json_get_string(&ret, "method", sv_null())));
            json_free(ret);
            continue;
        }
        if (json_has(&ret, "id") && json_get_int(&ret, "id", 0) != req.id) {
            // Response for different request. Drop on the floor
            trace(CAT_LSP, "Received response for request #%d while waiting for request #%d", json_get_int(&ret, "id", 0), req.id);
            json_free(ret);
            continue;
        }
        Response response = response_decode(&ret);
        assert(response.id == req.id);
        sv_free(request_json);
        RETURN(Response, response);
    }
}

void lsp_notification(char const *method, OptionalJSONValue params)
{
    Notification notification = { method, OptionalJSONValue_empty() };
    notification.params = params;
    assert(lsp);
    StringView  json = json_encode(notification_encode(&notification));
    char const *content_length = TextFormat("Content-Length: %zu\r\n\r\n", json.length + 2);
    write_pipe_write(&lsp->in, sv_from(content_length));
    write_pipe_write(&lsp->in, json);
    write_pipe_write(&lsp->in, sv_from("\r\n"));
    trace_json(params, "==|");
}

void lsp_initialize()
{
    if (lsp != NULL) {
        return;
    }
    lsp = process_create(sv_from("clangd"));
    lsp->stderr_file = sv_from("/tmp/clangd.log");
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
    Response          response = MUST(Response, lsp_message("initialize", params_json));
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
                    colors[i] = PI_KNOWN_IDENTIFIER;
                    break;
                case SemanticTokenTypesType:
                    colors[i] = PI_KNOWN_IDENTIFIER;
                    break;
                case SemanticTokenTypesClass:
                    colors[i] = PI_KNOWN_IDENTIFIER;
                    break;
                case SemanticTokenTypesEnum:
                    colors[i] = PI_KNOWN_IDENTIFIER;
                    break;
                case SemanticTokenTypesInterface:
                    colors[i] = PI_KNOWN_IDENTIFIER;
                    break;
                case SemanticTokenTypesStruct:
                    colors[i] = PI_KNOWN_IDENTIFIER;
                    break;
                case SemanticTokenTypesTypeParameter:
                    colors[i] = PI_KNOWN_IDENTIFIER;
                    break;
                case SemanticTokenTypesParameter:
                    colors[i] = PI_KNOWN_IDENTIFIER;
                    break;
                case SemanticTokenTypesVariable:
                    colors[i] = PI_KNOWN_IDENTIFIER;
                    break;
                case SemanticTokenTypesProperty:
                    colors[i] = PI_KNOWN_IDENTIFIER;
                    break;
                case SemanticTokenTypesEnumMember:
                    colors[i] = PI_KNOWN_IDENTIFIER;
                    break;
                case SemanticTokenTypesEvent:
                    colors[i] = PI_DEFAULT;
                    break;
                case SemanticTokenTypesFunction:
                    colors[i] = PI_FUNCTION;
                    break;
                case SemanticTokenTypesMethod:
                    colors[i] = PI_KNOWN_IDENTIFIER;
                    break;
                case SemanticTokenTypesMacro:
                    colors[i] = PI_KNOWN_IDENTIFIER;
                    break;
                case SemanticTokenTypesKeyword:
                    colors[i] = PI_KEYWORD;
                    break;
                case SemanticTokenTypesModifier:
                    colors[i] = PI_KEYWORD;
                    break;
                case SemanticTokenTypesComment:
                    colors[i] = PI_COMMENT;
                    break;
                case SemanticTokenTypesString:
                    colors[i] = PI_STRING;
                    break;
                case SemanticTokenTypesNumber:
                    colors[i] = PI_NUMBER;
                    break;
                case SemanticTokenTypesRegexp:
                    colors[i] = PI_STRING;
                    break;
                case SemanticTokenTypesOperator:
                    colors[i] = PI_DEFAULT;
                    break;
                case SemanticTokenTypesDecorator:
                    colors[i] = PI_KNOWN_IDENTIFIER;
                    break;
                default:
                    colors[i] = PI_STRING;
                }
            }
        }
    }
}

void lsp_on_open(int buffer_num)
{
    lsp_initialize();
    Buffer *buffer = eddy.buffers.elements + buffer_num;

    if (sv_empty(buffer->name)) {
        return;
    }
    DidOpenTextDocumentParams did_open = { 0 };
    did_open.textDocument = (TextDocumentItem) {
        .uri = buffer_uri(buffer),
        .languageId = sv_from("c"),
        .version = 0,
        .text = buffer->text.view
    };
    OptionalJSONValue did_open_json = DidOpenTextDocumentParams_encode(did_open);
    lsp_notification("textDocument/didOpen", did_open_json);
}

void lsp_semantic_tokens(int buffer_num)
{
    lsp_initialize();
    Buffer *buffer = eddy.buffers.elements + buffer_num;

    if (sv_empty(buffer->name)) {
        return;
    }
    SemanticTokensParams semantic_tokens_params = { 0 };
    semantic_tokens_params.textDocument = (TextDocumentIdentifier) {
        .uri = buffer_uri(buffer),
    };
    OptionalJSONValue semantic_tokens_params_json = SemanticTokensParams_encode(semantic_tokens_params);
    Response          response = MUST(Response, lsp_message("textDocument/semanticTokens/full", semantic_tokens_params_json));
    if (!response_success(&response)) {
        return;
    }
    if (!response.result.has_value) {
        trace(CAT_LSP, "No response to textDocument/semanticTokens/full");
        return;
    }
    OptionalSemanticTokens result_maybe = SemanticTokens_decode(response.result);
    if (!result_maybe.has_value) {
        trace(CAT_LSP, "Couldn't decode response to textDocument/semanticTokens/full");
        return;
    }
    SemanticTokens result = result_maybe.value;
    size_t         lineno = 0;
    Index         *line = buffer->lines.elements + lineno;
    size_t         offset = 0;
    UInt32s        data = result.data;
    for (size_t ix = 0; ix < result.data.size; ix += 5) {
        // trace(CAT_LSP, "Semantic token[%zu]: [%du, %du, %du]", ix, data.elements[ix], data.elements[ix + 1], data.elements[ix + 2]);
        if (data.elements[ix] > 0) {
            lineno += data.elements[ix];
            if (lineno >= buffer->lines.size) {
                // trace(CAT_LSP, "Semantic token[%zu] lineno %zu > buffer->lines %zu", ix, lineno, buffer->lines.size);
                continue;
            }
            line = buffer->lines.elements + lineno;
            offset = 0;
        }
        offset += data.elements[ix + 1];
        // trace(CAT_LSP, "Semantic token[%zu]: line: %zu col: %zu", ix, lineno, offset);
        size_t length = data.elements[ix + 2];
        for (size_t token_ix = 0; token_ix < line->num_tokens; ++token_ix) {
            assert(line->first_token + token_ix < buffer->tokens.size);
            DisplayToken *t = buffer->tokens.elements + line->first_token + token_ix;
            if (t->index == line->index_of + offset && t->length == length) {
                t->color = colors[data.elements[ix + 3]];
            }
        }
    }
}

void lsp_did_save(int buffer_num)
{
    lsp_initialize();
    Buffer *buffer = eddy.buffers.elements + buffer_num;

    if (sv_empty(buffer->name)) {
        return;
    }
    DidSaveTextDocumentParams did_save = { 0 };
    did_save.textDocument = (TextDocumentIdentifier) {
        .uri = buffer_uri(buffer),
    };
    did_save.text = OptionalStringView_create(buffer->text.view);
    OptionalJSONValue did_save_json = DidSaveTextDocumentParams_encode(did_save);
    lsp_notification("textDocument/didSave", did_save_json);
}

void lsp_did_close(int buffer_num)
{
    lsp_initialize();
    Buffer *buffer = eddy.buffers.elements + buffer_num;

    if (sv_empty(buffer->name)) {
        return;
    }
    DidCloseTextDocumentParams did_close = { 0 };
    did_close.textDocument = (TextDocumentIdentifier) {
        .uri = buffer_uri(buffer),
    };
    OptionalJSONValue did_close_json = DidCloseTextDocumentParams_encode(did_close);
    lsp_notification("textDocument/didClose", did_close_json);
}

void lsp_did_change(int buffer_num, IntVector2 start, IntVector2 end, StringView text)
{
    lsp_initialize();
    Buffer *buffer = eddy.buffers.elements + buffer_num;

    if (sv_empty(buffer->name)) {
        return;
    }
    DidChangeTextDocumentParams did_change = { 0 };
    did_change.textDocument.uri = buffer_uri(buffer);
    did_change.textDocument.version = buffer->version;
    TextDocumentContentChangeEvent contentChange = { 0 };
    contentChange._0.range.start.line = start.line;
    contentChange._0.range.start.character = start.column;
    contentChange._0.range.end.line = end.line;
    contentChange._0.range.end.character = end.column;
    contentChange._0.text = text;
    da_append_TextDocumentContentChangeEvent(&did_change.contentChanges, contentChange);
    OptionalJSONValue did_change_json = DidChangeTextDocumentParams_encode(did_change);
    lsp_notification("textDocument/didChange", did_change_json);
}

int lsp_format(int buffer_num)
{
    lsp_initialize();
    Buffer *buffer = eddy.buffers.elements + buffer_num;

    if (sv_empty(buffer->name)) {
        return -1;
    }

    DocumentFormattingParams params = { 0 };
    params.textDocument.uri = buffer_uri(buffer);
    params.options.insertSpaces = true;
    params.options.tabSize = 4;
    params.options.trimTrailingWhitespace = (OptionalBool) { .has_value = true, .value = true };
    params.options.insertFinalNewline = (OptionalBool) { .has_value = true, .value = true };
    params.options.trimFinalNewlines = (OptionalBool) { .has_value = true, .value = true };

    OptionalJSONValue params_json = DocumentFormattingParams_encode(params);
    Response          response = MUST(Response, lsp_message("textDocument/formatting", params_json));
    if (response_error(&response)) {
        return -1;
    }
    StringView s = json_encode(response.result.value);
    // trace(CAT_LSP, "Formatting edits: %.*s", SV_ARG(s));
    sv_free(s);

    if (response.result.value.type != JSON_TYPE_ARRAY) {
        return -1;
    }
    TextEdits edits = MUST_OPTIONAL(TextEdits, TextEdits_decode(response.result));
    for (size_t ix = 0; ix < edits.size; ++ix) {
        TextEdit   edit = edits.elements[ix];
        IntVector2 start = { edit.range.start.character, edit.range.start.line };
        IntVector2 end = { edit.range.end.character, edit.range.end.line };
        size_t     offset = buffer_position_to_index(buffer, start);
        size_t     length = buffer_position_to_index(buffer, end) - offset;

        if (sv_empty(edit.newText)) {
            buffer_delete(buffer, offset, length);
        } else if (length == 0) {
            buffer_insert(buffer, edit.newText, offset);
        } else {
            buffer_replace(buffer, offset, length, edit.newText);
        }
    }
    da_free_TextEdit(&edits);
    return edits.size;
}
