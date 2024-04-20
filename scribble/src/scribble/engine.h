/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef SCRIBBLE_ENGINE_H
#define SCRIBBLE_ENGINE_H

#include <base/http.h>
#include <base/io.h>
#include <base/json.h>
#include <scribble/type.h>

typedef enum execution_mode {
    EM_RUN = 0x00,
    EM_SINGLE_STEP = 0x01,
    EM_STEP_OVER = 0x02,
    EM_RUN_TO_RETURN = 0x04,
    EM_CONTINUE = 0x08,
} ExecutionMode;

typedef enum {
    EMT_STAGE_INIT,
    EMT_STAGE_START,
    EMT_PROGRAM_START,
    EMT_FUNCTION_ENTRY,
    EMT_ON_INSTRUCTION,
    EMT_AFTER_INSTRUCTION,
    EMT_FUNCTION_RETURN,
    EMT_PROGRAM_EXIT,
    EMT_STAGE_END,
} ExecutionMessageType;

typedef enum {
    ES_INIT,
    ES_PARSE,
    ES_BIND,
    ES_INTERMEDIATE,
    ES_GENERATE,
    ES_EXECUTE,
    ES_INTERPRET,
    ES_MAX,
} EngineStage;

typedef struct ExecutionMessage {
    ExecutionMessageType type;
    void                *payload;
} ExecutionMessage;

typedef struct command {
    StringView command;
    StringList arguments;
    StringView request;
} Command;

typedef struct {
    bool debug;
    bool execute;
} EngineStageConfig;

typedef struct backend_connection {
    socket_t    fd;
    StringView  socket;
    EngineStage stage;
    JSONValue   config;
    bool        threaded;
    void       *context;
} BackendConnection;

typedef bool (*EngineStageExecutor)(BackendConnection *, ExecutionMessage);
typedef bool (*FrontEndMessageHandler)(socket_t, HttpRequest, JSONValue);

extern int           scribble_backend(StringView path, bool threaded);
extern ErrorOrSocket start_backend_thread(StringView path);
extern ErrorOrSocket start_backend_process(StringView path);
extern void          scribble_frontend(JSONValue config, FrontEndMessageHandler handler);
bool                 frontend_message_handler(socket_t conn_fd, HttpRequest request, JSONValue config);

#endif /* SCRIBBLE_ENGINE_H */
