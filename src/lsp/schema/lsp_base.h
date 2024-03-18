/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __LSP_LSP_BASE_H__
#define __LSP_LSP_BASE_H__

#include "da.h"
#include <stdint.h>

#include <json.h>

typedef struct {
} Empty;
OPTIONAL(Empty)

typedef struct {
} Null;
OPTIONAL(Null)

typedef struct {
    char const       *method;
    OptionalJSONValue params;
} Notification;

typedef struct {
    int               id;
    char const       *method;
    OptionalJSONValue params;
} Request;

typedef struct {
    int               id;
    OptionalJSONValue result;
    OptionalJSONValue error;
} Response;

ERROR_OR(Response);

extern OptionalJSONValue   JSONValue_encode(JSONValue value);
extern OptionalJSONValue   JSONValue_decode(OptionalJSONValue value);
extern OptionalJSONValue   JSONValues_encode(JSONValues value);
extern OptionalJSONValues  JSONValues_decode(OptionalJSONValue json);
extern OptionalJSONValue   Int_encode(int value);
extern OptionalInt         Int_decode(OptionalJSONValue value);
extern OptionalJSONValue   Ints_encode(Ints value);
extern OptionalInts        Ints_decode(OptionalJSONValue value);
extern OptionalJSONValue   UInt32_encode(uint32_t value);
extern OptionalUInt32      UInt32_decode(OptionalJSONValue value);
extern OptionalJSONValue   UInt32s_encode(UInt32s value);
extern OptionalUInt32s     UInt32s_decode(OptionalJSONValue value);
extern OptionalJSONValue   Bool_encode(bool value);
extern OptionalBool        Bool_decode(OptionalJSONValue value);
extern OptionalJSONValue   StringView_encode(StringView value);
extern OptionalStringView  StringView_decode(OptionalJSONValue value);
extern OptionalJSONValue   StringViews_encode(StringViews value);
extern OptionalStringViews StringViews_decode(OptionalJSONValue value);
extern OptionalJSONValue   Empty_encode(Empty value);
extern OptionalEmpty       Empty_decode(OptionalJSONValue value);
extern OptionalJSONValue   Null_encode(Null value);
extern OptionalNull        Null_decode(OptionalJSONValue value);

extern JSONValue notification_encode(Notification *notification);
extern JSONValue request_encode(Request *request);
extern bool      response_success(Response *response);
extern bool      response_error(Response *response);
extern Response  response_decode(JSONValue *json);

#endif /* __LSP_LSP_BASE_H__ */
