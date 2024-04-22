/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LSP_LSP_H
#define LSP_LSP_H

#include <base/process.h>
#include <base/sv.h>
#include <lsp/schema/CompletionItem.h>
#include <lsp/schema/ServerCapabilities.h>

typedef struct {
    StringView        method;
    OptionalJSONValue params;
} Notification;

typedef struct {
    void             *sender;
    int               id;
    StringView        method;
    OptionalJSONValue params;
} Request;

DA_WITH_NAME(Request, Requests);

typedef struct {
    int               id;
    OptionalJSONValue result;
    OptionalJSONValue error;
} Response;

ERROR_OR(Response);

typedef struct lsp LSP;

typedef Process *(*LSPStart)(LSP *);
typedef struct mode *(*LSPInitMode)(LSP *);

typedef struct {
    LSPStart start;
} LSPHandlers;

typedef struct lsp {
    LSPHandlers        handlers;
    Condition          init_condition;
    Process           *lsp;
    bool               lsp_ready;
    ServerCapabilities server_capabilities;
    Requests           request_queue;
    StringScanner      lsp_scanner;
} LSP;

DA_WITH_NAME(LSP, LSPs);

extern JSONValue    notification_encode(Notification *notification);
extern Notification notification_decode(JSONValue *json);
extern void         notification_free(Notification *notification);
extern JSONValue    request_encode(Request *request);
extern void         request_free(Request *request);
extern bool         response_success(Response *response);
extern bool         response_error(Response *response);
extern Response     response_decode(JSONValue *json);
extern void         response_free(Response *response);
extern void         lsp_initialize(LSP *lsp);
extern void         lsp_initialize_theme(LSP *lsp);
extern ErrorOrInt   lsp_message(LSP *lsp, void *sender, char const *method, OptionalJSONValue params);
extern ErrorOrInt   lsp_notification(LSP *lsp, char const *method, OptionalJSONValue params);

#endif /* LSP_LSP_H */
