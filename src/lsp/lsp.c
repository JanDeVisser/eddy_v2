/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <sys/syslimits.h>
#include <unistd.h>

#define STATIC_ALLOCATOR
#include <allocate.h>
#include <eddy.h>
#include <palette.h>
#include <process.h>

#include <lsp/initialize.h>
#include <lsp/lsp_base.h>
#include <lsp/semantictokens.h>
#include <lsp/synchronization.h>

static Process           *lsp = NULL;
static ServerCapabilities server_capabilties;
static PaletteIndex colors[30];

ErrorOrResponse lsp_message(char const *method, OptionalJSONValue params)
{
    static int id = 0;
    Request    req = { .id = ++id, method, OptionalJSONValue_empty() };
    req.params = params;
    assert(lsp);
    {
        StringView json = json_encode(request_encode(&req));
        trace(CAT_LSP, "==> %.*s", SV_ARG(json));
        char const *content_length = TextFormat("Content-Length: %zu\r\n\r\n", json.length + 2);
        write_pipe_write(&lsp->in, sv_from(content_length));
        write_pipe_write(&lsp->in, json);
        write_pipe_write(&lsp->in, sv_from("\r\n"));
    }

    StringView buf = {0};
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
        size_t content_length = ss_read_number(&ss);
        if (!content_length) {
            continue;
        }
        if (!ss_expect_sv(&ss, sv_from("\r\n\r\n"))) {
            continue;
        }
        StringView json = ss_read(&ss, content_length);
        if (json.length < content_length) {
            continue;
        }
        trace(CAT_LSP, "<== %.*s", SV_ARG(json));
        buf = (StringView) {0};
        JSONValue ret = TRY_TO(JSONValue, Response, json_decode(json));
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
    trace(CAT_LSP, "==| %.*s", SV_ARG(json));
}

void lsp_initialize()
{
    if (lsp != NULL) {
        return;
    }
    lsp = process_create(sv_from("clangd"));
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
    params.rootUri = (OptionalStringView) { .has_value = true, .value = sv_from(prj) };
    params.capabilities.textDocument.has_value = true;

    StringList tokenTypes = {0};
    da_append_StringView(&tokenTypes, SemanticTokenTypes_to_string(SemanticTokenTypesComment));
    da_append_StringView(&tokenTypes, SemanticTokenTypes_to_string(SemanticTokenTypesKeyword));
    da_append_StringView(&tokenTypes, SemanticTokenTypes_to_string(SemanticTokenTypesVariable));
    da_append_StringView(&tokenTypes, SemanticTokenTypes_to_string(SemanticTokenTypesType));

    params.capabilities.textDocument.value.semanticTokens.has_value = true;
    params.capabilities.textDocument.value.semanticTokens.value.multilineTokenSupport = (OptionalBool) { .has_value = true, .value = true };
    params.capabilities.textDocument.value.semanticTokens.value.requests.has_value = true;
    params.capabilities.textDocument.value.semanticTokens.value.requests.full.has_value = true;
    params.capabilities.textDocument.value.semanticTokens.value.requests.full._0 = true;
    params.capabilities.textDocument.value.semanticTokens.value.tokenTypes = tokenTypes;

    OptionalJSONValue params_json = InitializeParams_encode(params);
    Response          response = MUST(Response, lsp_message("initialize", params_json));
    if (response_success(&response)) {
        InitializeResult result = InitializeResult_decode(response.result);
        if (result.serverInfo.has_value) {
            trace(CAT_LSP, "LSP server name: %.*s", SV_ARG(result.serverInfo.name));
            if (result.serverInfo.version.has_value) {
                trace(CAT_LSP, "LSP server version: %.*s", SV_ARG(result.serverInfo.version.value));
            }
        }
        server_capabilties = result.capabilities;
        assert(server_capabilties.semanticTokensProvider.has_value);
        trace(CAT_LSP, "#Token types: %zu", server_capabilties.semanticTokensProvider.value.legend.tokenTypes.size);
        for (int i = 0; i < server_capabilties.semanticTokensProvider.value.legend.tokenTypes.size; ++i) {
            StringView tokenType = server_capabilties.semanticTokensProvider.value.legend.tokenTypes.strings[i];
            SemanticTokenTypes semantic_token_type = SemanticTokenTypes_parse(tokenType);
            trace(CAT_LSP, "%d: '%.*s' %d", i, SV_ARG(tokenType), semantic_token_type);
            switch (semantic_token_type) {
                case SemanticTokenTypesNamespace: colors[i] = PI_KNOWN_IDENTIFIER; break;
                case SemanticTokenTypesType: colors[i] = PI_KNOWN_IDENTIFIER; break;
                case SemanticTokenTypesClass: colors[i] = PI_KNOWN_IDENTIFIER; break;
                case SemanticTokenTypesEnum: colors[i] = PI_KNOWN_IDENTIFIER; break;
                case SemanticTokenTypesInterface: colors[i] = PI_KNOWN_IDENTIFIER; break;
                case SemanticTokenTypesStruct: colors[i] = PI_KNOWN_IDENTIFIER; break;
                case SemanticTokenTypesTypeParameter: colors[i] = PI_KNOWN_IDENTIFIER; break;
                case SemanticTokenTypesParameter: colors[i] = PI_KNOWN_IDENTIFIER; break;
                case SemanticTokenTypesVariable: colors[i] = PI_KNOWN_IDENTIFIER; break;
                case SemanticTokenTypesProperty: colors[i] = PI_KNOWN_IDENTIFIER; break;
                case SemanticTokenTypesEnumMember: colors[i] = PI_KNOWN_IDENTIFIER; break;
                case SemanticTokenTypesEvent: colors[i] = PI_DEFAULT; break;
                case SemanticTokenTypesFunction: colors[i] = PI_KNOWN_IDENTIFIER; break;
                case SemanticTokenTypesMethod: colors[i] = PI_KNOWN_IDENTIFIER; break;
                case SemanticTokenTypesMacro: colors[i] = PI_KNOWN_IDENTIFIER; break;
                case SemanticTokenTypesKeyword: colors[i] = PI_KEYWORD; break;
                case SemanticTokenTypesModifier: colors[i] = PI_KEYWORD; break;
                case SemanticTokenTypesComment: colors[i] = PI_COMMENT; break;
                case SemanticTokenTypesString: colors[i] = PI_STRING; break;
                case SemanticTokenTypesNumber: colors[i] = PI_NUMBER; break;
                case SemanticTokenTypesRegexp: colors[i] = PI_STRING; break;
                case SemanticTokenTypesOperator: colors[i] = PI_DEFAULT; break;
                case SemanticTokenTypesDecorator: colors[i] = PI_KNOWN_IDENTIFIER; break;
            }
        }
    }
}

void lsp_on_open(int buffer_num)
{
    lsp_initialize();
    Buffer *buffer = eddy.buffers.elements + buffer_num;
    char    uri[PATH_MAX + 8];
    memset(uri, '\0', PATH_MAX + 8);
    snprintf(uri, PATH_MAX + 7, "file://%.*s/%.*s", SV_ARG(eddy.project_dir), SV_ARG(buffer->name));

    DidOpenTextDocumentParams did_open = { 0 };
    did_open.textDocument = (TextDocumentItem) {
        .uri = sv_from(uri),
        .languageId = sv_from("c"),
        .version = 0,
        .text = eddy.buffers.elements[buffer_num].text.view
    };
    OptionalJSONValue did_open_json = DidOpenTextDocumentParams_encode(did_open);
    lsp_notification("textDocument/didOpen", did_open_json);
}

void lsp_semantic_tokens(int buffer_num)
{
    lsp_initialize();
    Buffer *buffer = eddy.buffers.elements + buffer_num;
    char    uri[PATH_MAX + 8];
    memset(uri, '\0', PATH_MAX + 8);
    snprintf(uri, PATH_MAX + 7, "file://%.*s/%.*s", SV_ARG(eddy.project_dir), SV_ARG(buffer->name));

    SemanticTokensParams semantic_tokens_params = { 0 };
    semantic_tokens_params.textDocument = (TextDocumentIdentifier) {
        .uri = sv_from(uri),
    };
    OptionalJSONValue semantic_tokens_params_json = SemanticTokensParams_encode(semantic_tokens_params);
    Response          response = MUST(Response, lsp_message("textDocument/semanticTokens/full", semantic_tokens_params_json));
    if (response_success(&response)) {
        SemanticTokensResult result = SemanticTokensResult_decode(response.result);
        assert(result.tag == 0);
        size_t lineno = 0;
        Index *line = buffer->lines.elements + lineno;
        size_t offset = 0;
        UInts data = result._0.data;
        for (size_t ix = 0; ix < result._0.data.size; ix += 5) {
            if (data.elements[ix] > 0) {
                lineno += data.elements[ix];
                line = buffer->lines.elements + lineno;
                offset = 0;
            }
            offset += data.elements[ix+1];
            size_t length = data.elements[ix+2];
            for (size_t token_ix = 0; token_ix < line->num_tokens; ++token_ix) {
                DisplayToken *t = buffer->tokens.elements + line->first_token + token_ix;
                if (t->index == line->index_of + offset && t->length == length) {
                    t->color = colors[data.elements[ix+3]];
                }
            }
        }
    }
}
