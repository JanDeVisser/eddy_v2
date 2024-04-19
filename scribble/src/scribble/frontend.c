/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <unistd.h>

#include <base/http.h>
#include <base/process.h>
#include <scribble/engine.h>

ErrorOrSocket start_backend_process()
{
    StringView     path = sv_printf("/tmp/scribble-engine-%d", getpid());
    socket_t const listen_fd = MUST(Socket, unix_socket_listen(path));
    Process       *client = process_create(sv_from("scribble-backend"), sv_cstr(path, NULL));
    MUST(Int, process_start(client));
    trace(IPC, "Started client, pid = %d\n", client->pid);
    return socket_accept(listen_fd);
}

void *backend_main_wrapper(void *path)
{
    scribble_backend((StringView) { (char const *) path, strlen(path) }, true);
    return NULL;
}

ErrorOrSocket start_backend_thread()
{
    StringView     path = sv_printf("/tmp/scribble-engine-%d", getpid());
    socket_t const listen_fd = MUST(Socket, unix_socket_listen(path));
    pthread_t      thread;
    int            ret;

    if ((ret = pthread_create(&thread, NULL, backend_main_wrapper, (void *) path.ptr)) != 0) {
        fatal("Could not start backend thread: %s", strerror(ret));
    }
    trace(IPC, "Started client thread");
    return socket_accept(listen_fd);
}

void scribble_frontend(JSONValue config, FrontEndMessageHandler handler)
{
    socket_t socket = { 0 };
    if (json_get_bool(&config, "threaded", false)) {
        socket = MUST(Socket, start_backend_thread());
    } else {
        socket = MUST(Socket, start_backend_process());
    }

    while (true) {
        trace(IPC, "[S] Waiting for request");
        HttpRequest request = MUST(HttpRequest, http_request_receive(socket));
        if (handler(socket, request, config)) {
            break;
        }
    }
    socket_close(socket);
}

bool handle_parser_message(socket_t socket, HttpRequest request)
{
    if (sv_eq_cstr(request.url, "/parser/start")) {
        printf("[parser] started\n");
        HTTP_RESPONSE_OK(socket);
        return false;
    }
    if (sv_eq_cstr(request.url, "/parser/done")) {
        printf("[parser] done\n");
        HTTP_RESPONSE_OK(socket);
        return false;
    }
    if (sv_eq_cstr(request.url, "/parser/info")) {
        JSONValue msg = MUST(JSONValue, json_decode(request.body));
        printf("[parser] %.*s\n", SV_ARG(msg.string));
        HTTP_RESPONSE_OK(socket);
        return false;
    }
    if (sv_eq_cstr(request.url, "/parser/node")) {
        JSONValue  node = MUST(JSONValue, json_decode(request.body));
        StringView type = json_get_string(&node, "type", sv_null());
        StringView name = json_get_string(&node, "name", sv_null());
        printf("[parser] %.*s %.*s\n", SV_ARG(type), SV_ARG(name));
        HTTP_RESPONSE_OK(socket);
        return false;
    }
    if (sv_eq_cstr(request.url, "/parser/errors")) {
        JSONValue  node = MUST(JSONValue, json_decode(request.body));
        if (node.type == JSON_TYPE_ARRAY) {
            for (size_t ix = 0; ix < json_len(&node); ++ix) {
                JSONValue error = MUST_OPTIONAL(JSONValue, json_at(&node, ix));
                assert(error.type == JSON_TYPE_STRING);
                printf("ERROR: %.*s\n", SV_ARG(error.string));
            }
        } else {
            printf("[parser] ERRORS:\n%.*s\n", SV_ARG(request.body));
        }
        HTTP_RESPONSE_OK(socket);
        return true;
    }
    HTTP_RESPONSE(socket, HTTP_STATUS_NOT_FOUND);
    return false;
}

void handle_bind_message(socket_t socket, HttpRequest request)
{
    if (sv_eq_cstr(request.url, "/bind/start")) {
        printf("[bind] started\n");
        HTTP_RESPONSE_OK(socket);
        return;
    }
    if (sv_eq_cstr(request.url, "/bind/done")) {
        JSONValue iter_stats = MUST(JSONValue, json_decode(request.body));
        int       iteration = json_get_int(&iter_stats, "iteration", 1);
        int       warnings = json_get_int(&iter_stats, "warnings", 0);
        int       total_warnings = json_get_int(&iter_stats, "total_warnings", 0);
        int       errors = json_get_int(&iter_stats, "errors", 0);
        int       unbound = json_get_int(&iter_stats, "unbound", 0);
        printf("[bind] Done! Iteration %d: %d warning(s), %d total warning(s), %d error(s), %d unbound node(s)\n", iteration, warnings, total_warnings, errors, unbound);
        HTTP_RESPONSE_OK(socket);
        return;
    }
    if (sv_eq_cstr(request.url, "/bind/info")) {
        JSONValue msg = MUST(JSONValue, json_decode(request.body));
        printf("[bind] %.*s\n", SV_ARG(msg.string));
        HTTP_RESPONSE_OK(socket);
        return;
    }
    if (sv_eq_cstr(request.url, "/bind/syntaxnode")) {
        JSONValue  node = MUST(JSONValue, json_decode(request.body));
        StringView type = json_get_string(&node, "nodetype", sv_null());
        StringView name = json_get_string(&node, "name", sv_null());
        printf("[bind] %.*s %.*s\n", SV_ARG(type), SV_ARG(name));
        HTTP_RESPONSE_OK(socket);
        return;
    }
    if (sv_eq_cstr(request.url, "/bind/boundnode")) {
        JSONValue  node = MUST(JSONValue, json_decode(request.body));
        StringView node_type = json_get_string(&node, "nodetype", sv_null());
        StringView name = json_get_string(&node, "name", sv_null());
        StringView type = json_get_string(&node, "type", sv_null());
        printf("[bind] %.*s %.*s : %.*s\n", SV_ARG(node_type), SV_ARG(name), SV_ARG(type));
        HTTP_RESPONSE_OK(socket);
        return;
    }
    if (sv_eq_cstr(request.url, "/bind/iteration")) {
        JSONValue iter_stats = MUST(JSONValue, json_decode(request.body));
        int       iteration = json_get_int(&iter_stats, "iteration", 1);
        int       warnings = json_get_int(&iter_stats, "warnings", 0);
        int       total_warnings = json_get_int(&iter_stats, "total_warnings", 0);
        int       errors = json_get_int(&iter_stats, "errors", 0);
        int       unbound = json_get_int(&iter_stats, "unbound", 0);
        printf("[bind] Iteration %d: %d warning(s), %d total warning(s), %d error(s), %d unbound node(s)\n", iteration, warnings, total_warnings, errors, unbound);
        HTTP_RESPONSE_OK(socket);
        return;
    }
    if (sv_eq_cstr(request.url, "/bind/error")) {
        JSONValue iter_stats = MUST(JSONValue, json_decode(request.body));
        int       iteration = json_get_int(&iter_stats, "iteration", 1);
        int       warnings = json_get_int(&iter_stats, "warnings", 0);
        int       total_warnings = json_get_int(&iter_stats, "total_warnings", 0);
        int       errors = json_get_int(&iter_stats, "errors", 0);
        int       unbound = json_get_int(&iter_stats, "unbound", 0);
        printf("[bind] ERROR: Iteration %d: %d warning(s), %d total warning(s), %d error(s), %d unbound node(s)\n", iteration, warnings, total_warnings, errors, unbound);
        HTTP_RESPONSE_OK(socket);
        return;
    }
    HTTP_RESPONSE(socket, HTTP_STATUS_NOT_FOUND);
}

void handle_intermediate_message(socket_t socket, HttpRequest request)
{
    if (sv_eq_cstr(request.url, "/intermediate/start")) {
        printf("[ir] started\n");
        HTTP_RESPONSE_OK(socket);
        return;
    }
    if (sv_eq_cstr(request.url, "/intermediate/done")) {
        printf("[ir] done\n");
        HTTP_RESPONSE_OK(socket);
        return;
    }
    HTTP_RESPONSE(socket, HTTP_STATUS_NOT_FOUND);
}

void handle_execute_message(socket_t socket, HttpRequest request)
{
    if (sv_eq_cstr(request.url, "/execute/start")) {
        printf("[execute] started\n");
        HTTP_RESPONSE_OK(socket);
        return;
    }
    if (sv_eq_cstr(request.url, "/execute/done")) {
        printf("[execute] done\n");
        HTTP_RESPONSE_OK(socket);
        return;
    }
    HTTP_RESPONSE(socket, HTTP_STATUS_NOT_FOUND);
}

void handle_generate_message(socket_t socket, HttpRequest request)
{
    if (sv_eq_cstr(request.url, "/generate/start")) {
        printf("[generate] started\n");
        HTTP_RESPONSE_OK(socket);
        return;
    }
    if (sv_eq_cstr(request.url, "/generate/done")) {
        printf("[generate] done\n");
        HTTP_RESPONSE_OK(socket);
        return;
    }
    HTTP_RESPONSE(socket, HTTP_STATUS_NOT_FOUND);
}

void handle_arm64_message(socket_t socket, HttpRequest request)
{
    if (sv_eq_cstr(request.url, "/arm64/generate/start")) {
        printf("[arm64] started\n");
        HTTP_RESPONSE_OK(socket);
        return;
    }
    if (sv_eq_cstr(request.url, "/arm64/generate/done")) {
        printf("[arm64] done\n");
        HTTP_RESPONSE_OK(socket);
        return;
    }
    if (sv_eq_cstr(request.url, "/arm64/generate/function/entry")) {
        JSONValue json = MUST(JSONValue, json_decode(request.body));
        printf("[arm64] function %.*s\n", SV_ARG(json_get_string(&json, "name", SV("--"))));
        HTTP_RESPONSE_OK(socket);
        return;
    }
    if (sv_eq_cstr(request.url, "/arm64/generate/function/exit")) {
        JSONValue json = MUST(JSONValue, json_decode(request.body));
        printf("[arm64] function %.*s done\n", SV_ARG(json_get_string(&json, "name", SV("--"))));
        HTTP_RESPONSE_OK(socket);
        return;
    }
    HTTP_RESPONSE(socket, HTTP_STATUS_NOT_FOUND);
}

bool frontend_message_handler(socket_t conn_fd, HttpRequest request, JSONValue config)
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
    if (sv_startswith(request.url, sv_from("/parser/"))) {
        return handle_parser_message(conn_fd, request);
    }
    if (sv_startswith(request.url, sv_from("/bind/"))) {
        handle_bind_message(conn_fd, request);
        return false;
    }
    if (sv_startswith(request.url, sv_from("/intermediate/"))) {
        handle_intermediate_message(conn_fd, request);
        return false;
    }
    if (sv_startswith(request.url, sv_from("/execute/"))) {
        handle_execute_message(conn_fd, request);
        return false;
    }
    if (sv_startswith(request.url, sv_from("/generate/"))) {
        handle_generate_message(conn_fd, request);
        return false;
    }
    if (sv_startswith(request.url, sv_from("/arm64/"))) {
        handle_arm64_message(conn_fd, request);
        return false;
    }
    if (sv_eq_cstr(request.url, "/goodbye")) {
        HttpResponse response = { 0 };
        response.status = HTTP_STATUS_OK;
        http_response_send(conn_fd, &response);
        return true;
    }
    HttpResponse response = { 0 };
    response.status = HTTP_STATUS_NOT_FOUND;
    http_response_send(conn_fd, &response);
    return false;
}
