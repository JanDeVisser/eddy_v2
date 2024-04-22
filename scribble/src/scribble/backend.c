/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */


#include <pthread.h>
#include <unistd.h>

#include <base/fs.h>
#include <base/http.h>
#include <arch/arm64/arm64.h>
#include <binder.h>
#include <config.h>
#include <engine.h>
#include <execute.h>
#include <graph.h>
#include <intermediate.h>
#include <model/error.h>
#include <parser.h>
#include <type.h>

noreturn void shutdown_backend(BackendConnection *conn, int code)
{
    socket_close(conn->fd);
    fs_unlink(conn->socket);
    if (!conn->threaded) {
        exit(1);
    } else {
        pthread_exit(NULL);
    }
}

noreturn void exit_backend(BackendConnection *conn, StringView error)
{
    HTTP_POST_MUST(conn->fd, "/panic", json_string(sv_printf("Error: %.*s", SV_ARG(error))));
    shutdown_backend(conn, 1);
}

bool bootstrap_backend(BackendConnection *conn)
{
    if (http_get_message(conn->fd, sv_from("/hello"), (StringList) { 0 }) != HTTP_STATUS_HELLO) {
        fatal("/hello failed");
    }
    conn->config = HTTP_GET_REQUEST_MUST(conn->fd, "/bootstrap/config", (StringList) { 0 });
    printf("[C] Got config\n");
    return true;
}

OptionalJSONValue get_stage(JSONValue *stages, char const *stage)
{
    trace(BACKEND, "Looking for stage '%s'", stage);
    for (size_t ix = 0; ix < json_len(stages); ++ix) {
        JSONValue  s = MUST_OPTIONAL(JSONValue, json_at(stages, ix));
        if (s.type != JSON_TYPE_OBJECT) {
            continue;
        }
        StringView name = json_get_string(&s, "name", sv_null());
        trace(BACKEND, "--> '%.*s'", SV_ARG(name));
        if (sv_eq_cstr(name, stage)) {
            trace(BACKEND, "Found it");
            RETURN_VALUE(JSONValue, s);
        }
    }
    trace(BACKEND, "Not there");
    RETURN_EMPTY(JSONValue);
}

void compile_program(BackendConnection *conn)
{
    type_registry_init();
    JSONValue stages = MUST_OPTIONAL(JSONValue, json_get(&conn->config, "stages"));
    assert(stages.type == JSON_TYPE_ARRAY);
    SyntaxNode *program = NULL;
    BoundNode  *ast = NULL;
    IRProgram   ir = { 0 };

    OptionalJSONValue stage = get_stage(&stages, "parse");
    if (!stage.has_value) {
        StringList errors = {0};
        sl_push(&errors, sv_printf("error=%s", "NO_PARSE_STAGE"));
        http_get_message(conn->fd, sv_from("/panic"), errors);
        exit_backend(conn, SV("No parse stage"));
    }
    ParserContext parse_result = parse(conn, stage.value);
    if (parse_result.errors.size > 0) {
        JSONValue errors = scribble_errors_to_json(&parse_result.errors);
        HTTP_POST_MUST(conn->fd, "/parser/errors", errors);
        exit_backend(conn, sv_from("Parse errors found"));
    }
    if (json_get_bool(&stage.value, "graph", false)) {
        graph_program(parse_result.program);
    }
    program = parse_result.program;

    stage = get_stage(&stages, "bind");
    if (!stage.has_value) {
        exit_backend(conn, SV("No bind stage"));
    }
    assert(program != NULL);
    bool debug = json_get_bool(&stage.value, "debug", false);
    ast = bind_program(conn, stage.value, program);
    if (ast == NULL) {
        return;
    }

    stage = get_stage(&stages, "ir");
    if (!stage.has_value) {
        exit_backend(conn, SV("No IR stage"));
    }
    debug = json_get_bool(&stage.value, "debug", false);
    if (debug) {
        HTTP_GET_MUST(conn->fd, "/intermediate/start", sl_create());
    }
    ir = generate(conn, ast);
    if (debug) {
        HTTP_GET_MUST(conn->fd, "/intermediate/done", sl_create());
    }
    stage = get_stage(&stages, "generate");
    if (stage.has_value) {
        debug = json_get_bool(&stage.value, "debug", false);
        if (debug) {
            HTTP_GET_MUST(conn->fd, "/generate/start", sl_create());
        }
        output_arm64(conn, &ir);
        if (debug) {
            HTTP_GET_MUST(conn->fd, "/generate/done", sl_create());
        }
    }
    stage = get_stage(&stages, "execute");
    if (stage.has_value) {
        ErrorOrInt exit_code = execute(conn, ir);
        if (ErrorOrInt_has_value(exit_code)) {
            HTTP_GET_MUST(conn->fd, "/goodbye", sl_create());
            return;
        }
        HTTP_POST_MUST(conn->fd, "/error", json_string(sv_from(Error_to_string(exit_code.error))));
    }
    stage = get_stage(&stages, "execute");
    if (stage.has_value) {
        debug = json_get_bool(&stage.value, "debug", false);
        if (debug) {
            HTTP_GET_MUST(conn->fd, "/execute/start", sl_create());
        }
        execute(conn, ir);
        if (debug) {
            HTTP_GET_MUST(conn->fd, "/execute/done", sl_create());
        }
    }
    exit_backend(conn, SV("0"));
}

int scribble_backend(StringView path, bool threaded)
{
    socket_t          conn_fd = MUST(Socket, unix_socket_connect(path));
    BackendConnection conn = { 0 };

    conn.fd = conn_fd;
    conn.context = NULL;
    conn.socket = path;
    conn.threaded = threaded;

    if (http_get_message(conn.fd, sv_from("/hello"), (StringList) { 0 }) != HTTP_STATUS_HELLO) {
        fatal("/hello failed");
    }
    conn.config = HTTP_GET_REQUEST_MUST(conn.fd, "/bootstrap/config", (StringList) { 0 });
    trace(IPC, "[C] Got config");

    compile_program(&conn);

    trace(IPC, "[C] Closing socket");
    socket_close(conn_fd);

    return 0;
}
