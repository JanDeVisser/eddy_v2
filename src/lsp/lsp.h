/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __LSP_LSP_H__
#define __LSP_LSP_H__

#include <app/buffer.h>
#include <app/editor.h>
#include <app/widget.h>
#include <base/sv.h>
#include <lsp/schema/CompletionItem.h>

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

typedef struct {
    int               id;
    OptionalJSONValue result;
    OptionalJSONValue error;
} Response;

ERROR_OR(Response);

extern JSONValue    notification_encode(Notification *notification);
extern Notification notification_decode(JSONValue *json);
extern void         notification_free(Notification *notification);
extern JSONValue    request_encode(Request *request);
extern void         request_free(Request *request);
extern bool         response_success(Response *response);
extern bool         response_error(Response *response);
extern Response     response_decode(JSONValue *json);
extern void         response_free(Response *response);
extern void         lsp_initialize();
extern ErrorOrInt   lsp_message(void *sender, char const *method, OptionalJSONValue params);
extern ErrorOrInt   lsp_notification(char const *method, OptionalJSONValue params);

#endif /* __LSP_LSP_H__ */
