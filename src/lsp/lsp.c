/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <unistd.h>

#define STATIC_ALLOCATOR
#include <allocate.h>
#include <eddy.h>

#include <lsp/initialize.h>
#include <lsp/lsp_base.h>
#include <lsp/semantictokens.h>
#include <lsp/synchronization.h>
#include <process.h>
#include <sys/syslimits.h>

static Process           *lsp = NULL;
static ServerCapabilities server_capabilties;

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

    SemanticTokensParams semantic_tokens_params = { 0 };
    semantic_tokens_params.textDocument = (TextDocumentIdentifier) {
        .uri = sv_from(uri),
    };
    OptionalJSONValue semantic_tokens_params_json = SemanticTokensParams_encode(semantic_tokens_params);
    Response          response = MUST(Response, lsp_message("textDocument/semanticTokens/full", semantic_tokens_params_json));
    if (response_success(&response)) {
        SemanticTokensResult result = SemanticTokensResult_decode(response.result);
        assert(result.tag == 0);
        fprintf(stderr, "GOT %zu TOKENS\n", result._0.data.size);
    }
}
