/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <base/options.h>
#include <app/buffer.h>
#include <app/eddy.h>
#include <app/editor.h>
#include <app/scribble.h>
#include <app/widget.h>
#include <scribble/engine.h>

// -- Scribble Mode ---------------------------------------------------------

MODE_CLASS_DEF(ScribbleMode, scribble_mode);

bool message_handler(socket_t conn_fd, HttpRequest request, JSONValue config)
{
    trace(IPC, "[S] Got %.*s", SV_ARG(request.url));
    if (sv_eq_cstr(request.url, "/hello")) {
        HttpResponse response = { 0 };
        response.status = HTTP_STATUS_HELLO;
        http_response_send(conn_fd, &response);
        return false;
    }
    if (sv_eq_cstr(request.url, "/bootstrap/config")) {
        HttpResponse response = { 0 };
        response.status = HTTP_STATUS_OK;
        response.body = json_encode(config);
        http_response_send(conn_fd, &response);
        return false;
    }
    if (sv_eq_cstr(request.url, "/execute/function/entry")) {
        JSONValue func = MUST(JSONValue, json_decode(request.body));
        trace(IPC, "[function/entry] %.*s", SV_ARG(json_get_string(&func, "name", sv_null())));
        HttpResponse response = { 0 };
        response.status = HTTP_STATUS_OK;
        response.body = json_encode(json_bool(true));
        http_response_send(conn_fd, &response);
        return false;
    }
    if (sv_eq_cstr(request.url, "/execute/function/exit")) {
        JSONValue func = MUST(JSONValue, json_decode(request.body));
        trace(IPC, "[function/exit] %.*s", SV_ARG(json_get_string(&func, "name", sv_null())));
        HttpResponse response = { 0 };
        response.status = HTTP_STATUS_OK;
        response.body = json_encode(json_bool(true));
        http_response_send(conn_fd, &response);
        return false;
    }
    if (sv_eq_cstr(request.url, "/execute/function/on")) {
        HttpResponse response = { 0 };
        response.status = HTTP_STATUS_OK;
        response.body = json_encode(json_bool(true));
        http_response_send(conn_fd, &response);
        return false;
    }
    if (sv_eq_cstr(request.url, "/execute/function/after")) {
        HttpResponse response = { 0 };
        response.status = HTTP_STATUS_OK;
        response.body = json_encode(json_bool(true));
        http_response_send(conn_fd, &response);
        return false;
    }
    if (sv_eq_cstr(request.url, "/goodbye")) {
        HttpResponse response = { 0 };
        response.status = HTTP_STATUS_OK;
        http_response_send(conn_fd, &response);
        return true;
    }
    HttpResponse response = { 0 };
    response.status = HTTP_STATUS_OK;
    http_response_send(conn_fd, &response);
    return false;
}

void scribble_cmd_execute(ScribbleMode *mode, JSONValue unused)
{
    Editor     *editor = (Editor *) mode->parent->parent;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;

    JSONValue config = json_object();
    json_set(&config, "threaded", json_bool(has_option("threaded")));
    JSONValue stages = json_array();
    JSONValue stage = json_object();
    json_set_cstr(&stage, "name", "parse");
    json_set_string(&stage, "buffer_name", buffer->name);
    json_set_string(&stage, "text", buffer->text.view);
    json_set(&stage, "debug", json_bool(true));
    json_append(&stages, stage);
    stage = json_object();
    json_set_cstr(&stage, "name", "bind");
    json_set(&stage, "debug", json_bool(true));
    json_append(&stages, stage);
    stage = json_object();
    json_set_cstr(&stage, "name", "ir");
    json_set(&stage, "debug", json_bool(true));
    json_append(&stages, stage);
    stage = json_object();
    json_set_cstr(&stage, "name", "execute");
    json_set(&stage, "debug", json_bool(true));
    json_append(&stages, stage);
    json_set(&config, "stages", stages);
    scribble_frontend(config, message_handler);
}

void scribble_mode_on_draw(ScribbleMode *mode)
{
}

void scribble_mode_buffer_event_listener(Buffer *buffer, BufferEvent event)
{
    switch (event.type) {
    case ETInsert:
    case ETDelete:
    case ETReplace:
    case ETIndexed:
    case ETSave:
    case ETClose:
    default:
        break;
    }
}

void scribble_mode_init(ScribbleMode *mode)
{
    widget_add_command(mode, "scribble-split-line", (WidgetCommandHandler) mode_cmd_split_line,
        (KeyCombo) { KEY_ENTER, KMOD_NONE }, (KeyCombo) { KEY_KP_ENTER, KMOD_NONE });
    widget_add_command(mode, "scribble-unindent", (WidgetCommandHandler) mode_cmd_unindent,
        (KeyCombo) { KEY_TAB, KMOD_SHIFT });
    widget_register(mode, "execute-buffer", (WidgetCommandHandler) scribble_cmd_execute);
    // mode->handlers.on_draw = (WidgetOnDraw) scribble_mode_on_draw;
    mode->language = &scribble_language;
    BufferView *view = (BufferView *) mode->parent;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    buffer_add_listener(buffer, scribble_mode_buffer_event_listener);
}

typedef enum {
    ScribbleDirectiveStateInit = 0,
    ScribbleDirectiveStateIncludeQuote,
    ScribbleDirectiveStateMacroName,
} ScribbleDirectiveState;

static int handle_include_directive(Lexer *lexer)
{
    char const *buffer = lexer_source(lexer).ptr;
    size_t      ix = 0;
    size_t      state = (size_t) lexer->language_data;
    switch (state) {
    case ScribbleDirectiveStateInit: {
        while (buffer[ix] == ' ' || buffer[ix] == '\t') {
            ++ix;
        }
        lexer->language_data = (void *) ScribbleDirectiveStateIncludeQuote;
        if (ix > 0) {
            lexer_set_current(lexer, (Token) { .kind = TK_WHITESPACE, .text = { buffer, ix } });
            return ScribbleDirectiveInclude;
        }
        lexer->language_data = (void *) ScribbleDirectiveStateIncludeQuote;
    } // Fall through
    case ScribbleDirectiveStateIncludeQuote: {
        if (buffer[0] != '<' && buffer[0] != '"') {
            lexer->language_data = NULL;
            return NO_DIRECTIVE;
        }
        ++ix;
        while (buffer[ix] && buffer[ix] != '"' && buffer[ix] != '\n') {
            ++ix;
        }
        lexer_set_current(lexer, (Token) { .kind = TK_DIRECTIVE_ARG, .text = { buffer, ix + 1 } });
    } break;
    default:
        UNREACHABLE();
    }
    lexer->language_data = NULL;
    return NO_DIRECTIVE;
}

int handle_scribble_directive(Lexer *lexer, int directive)
{
    switch (directive) {
    case ScribbleDirectiveInclude:
        return handle_include_directive(lexer);
    default:
        lexer->language_data = (void *) NULL;
        return NO_DIRECTIVE;
    }
}
