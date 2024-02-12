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

Response lsp_message(char const *method, OptionalJSONValue params)
{
    static int id = 0;
    Request    req = { .id = ++id, method, OptionalJSONValue_empty() };
    req.params = params;
    assert(lsp);
    StringView  json = json_encode(request_encode(&req));
    fprintf(stderr, "-> LSP %.*s\n", SV_ARG(json));
    char const *content_length = TextFormat("Content-Length: %zu\r\n\r\n", json.length + 2);
    write_pipe_write(&lsp->in, sv_from(content_length));
    write_pipe_write(&lsp->in, json);
    write_pipe_write(&lsp->in, sv_from("\r\n"));

    read_pipe_expect(&lsp->out);
    StringList lines = read_pipe_lines(&lsp->out);
    if (lines.size != 3) {
        fprintf(stderr, "LSP replied with %zu lines instead of 3\n", lines.size);
        return (Response) { 0 };
    }
    if (!sv_startswith(lines.strings[0], sv_from("Content-Length"))) {
        fprintf(stderr, "LSP replied with %.*s instead of 'Content-Length'\n", SV_ARG(lines.strings[0]));
        return (Response) { 0 };
    }
    if (!sv_empty(lines.strings[1])) {
        fprintf(stderr, "LSP replied with %.*s instead of empty line\n", SV_ARG(lines.strings[1]));
        return (Response) { 0 };
    }
    fprintf(stderr, "LSP -> %.*s\n", SV_ARG(lines.strings[2]));
    ErrorOrJSONValue ret_maybe = json_decode(lines.strings[2]);
    if (ErrorOrJSONValue_is_error(ret_maybe)) {
        fprintf(stderr, "Could not JSON decode LSP reponse '%.*s': %s\n", SV_ARG(lines.strings[2]), Error_to_string(ret_maybe.error));
        return (Response) { 0 };
    }
    Response response = response_decode(&ret_maybe.value);
    assert(response.id == req.id);
    return response;
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
    fprintf(stderr, "-| LSP %.*s\n", SV_ARG(json));
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
    Response          response = lsp_message("initialize", params_json);
    if (response_success(&response)) {
        InitializeResult result = InitializeResult_decode(response.result);
        if (result.serverInfo.has_value) {
            fprintf(stderr, "LSP server name: %.*s\n", SV_ARG(result.serverInfo.name));
            if (result.serverInfo.version.has_value) {
                fprintf(stderr, "LSP server version: %.*s\n", SV_ARG(result.serverInfo.version.value));
            }
        }
        server_capabilties = result.capabilities;
    }
}

void lsp_on_open(int buffer_num)
{
    lsp_initialize();
    Buffer *buffer = eddy.buffers.elements + buffer_num;

    char                 uri[PATH_MAX + 8];
    memset(uri, '\0', PATH_MAX + 8);
    snprintf(uri, PATH_MAX + 7, "file://%.*s/%.*s", SV_ARG(eddy.project_dir), SV_ARG(buffer->name));

    DidOpenTextDocumentParams did_open = {0};
    did_open.textDocument = (TextDocumentItem) {
        .uri = sv_from(uri),
        .languageId = sv_from("c"),
        .version = 0,
        .text = eddy.buffers.elements[buffer_num].text.view
    };
    OptionalJSONValue did_open_json = DidOpenTextDocumentParams_encode(did_open);
    lsp_notification("textDocument/didOpen", did_open_json);

    SemanticTokensParams semantic_tokens_params = {0};
    semantic_tokens_params.textDocument = (TextDocumentIdentifier) {
        .uri = sv_from(uri),
    };
    OptionalJSONValue semantic_tokens_params_json = SemanticTokensParams_encode(semantic_tokens_params);
    Response          response = lsp_message("textDocument/semanticTokens/full", semantic_tokens_params_json);
    if (response_success(&response)) {
        SemanticTokensResult result = SemanticTokensResult_decode(response.result);
        assert(result.tag == 0);
        fprintf(stderr, "GOT %zu TOKENS\n", result._0.data.size);
    }
}
