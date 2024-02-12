/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __LSP_LSP_BASE_H__
#define __LSP_LSP_BASE_H__

#include <json.h>
#include <lsp.h>

#define JSON_ENCODE(T, A) extern OptionalJSONValue A##_encode(T value);
#define JSON_DECODE(T, A) extern T A##_decode(OptionalJSONValue json);
#define JSON(T, A)    \
    JSON_ENCODE(T, A) \
    JSON_DECODE(T, A)

typedef struct {
} Empty;
JSON(Empty, Empty)

typedef struct {
} Null;
JSON(Null, Null)

#define OPTIONAL_JSON_ENCODE(T) JSON_ENCODE(Optional##T, Optional##T)
#define OPTIONAL_JSON_DECODE(T) JSON_DECODE(Optional##T, Optional##T)
#define OPTIONAL_JSON(T) \
    OPTIONAL_JSON_ENCODE(T) \
    OPTIONAL_JSON_DECODE(T)

#define OPTIONAL_JSON_ENCODE_IMPL(T)                          \
    OptionalJSONValue Optional##T##_encode(Optional##T value) \
    {                                                         \
        if (value.has_value) {                                \
            return T##_encode(value.value);                   \
        } else {                                              \
            RETURN_EMPTY(JSONValue);                          \
        }                                                     \
    }
#define OPTIONAL_JSON_DECODE_IMPL(T)                                               \
    Optional##T Optional##T##_decode(OptionalJSONValue json)                       \
    {                                                                              \
        if (json.has_value) {                                                      \
            return (Optional##T) { .has_value = true, .value = T##_decode(json) }; \
        } else {                                                                   \
            return (Optional##T) { 0 };                                            \
        }                                                                          \
    }
#define OPTIONAL_JSON_IMPL(T)    \
    OPTIONAL_JSON_ENCODE_IMPL(T) \
    OPTIONAL_JSON_DECODE_IMPL(T)

#define DA_JSON_ENCODE_IMPL(T, A, E)                          \
    OptionalJSONValue A##_encode(A value)                     \
    {                                                         \
        JSONValue ret = json_array();                         \
        for (size_t ix = 0; ix < value.size; ++ix) {          \
            json_append(&ret, T##_encode(value.E[ix]).value); \
        }                                                     \
        RETURN_VALUE(JSONValue, ret);                         \
    }
#define DA_JSON_DECODE_IMPL(T, A, E)                            \
    A A##_decode(OptionalJSONValue json)                        \
    {                                                           \
        assert(json.has_value);                                 \
        assert(json.value.type == JSON_TYPE_ARRAY);             \
        A ret = {0};                                            \
        for (size_t ix = 0; ix < json_len(&json.value); ++ix) { \
            OptionalJSONValue elem = json_at(&json.value, ix);  \
            da_append_##T(&ret, T##_decode(elem));              \
        }                                                       \
        return ret;                                             \
    }
#define DA_JSON_IMPL(T, A, E)    \
    DA_JSON_ENCODE_IMPL(T, A, E) \
    DA_JSON_DECODE_IMPL(T, A, E)

#define OptionalJSONValue_encode(V) (V)
#define OptionalJSONValue_decode(V) (V)
#define JSONValue_encode(V) (V)

typedef StringView         URI;
typedef OptionalStringView OptionalURI;
typedef StringView         DocumentUri;
typedef OptionalStringView OptionalDocumentUri;

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

#define Int_encode(V) ((OptionalJSONValue) { .has_value = true, .value = json_int(V) })
#define Int_decode(V) ({ assert(V.has_value); assert(V.value.type == JSON_TYPE_INT); json_int_value(V.value); })
#define UInt_encode(V) ((OptionalJSONValue) { .has_value = true, .value = json_int(V) })
#define UInt_decode(V) ({ assert(V.has_value); assert(V.value.type == JSON_TYPE_INT); (uint32_t) json_int_value(V.value); })
#define Bool_encode(V) ((OptionalJSONValue) { .has_value = true, .value = json_bool(V) })
#define Bool_decode(V) ({ assert(V.has_value); assert(V.value.type == JSON_TYPE_BOOLEAN); V.value.boolean; })
#define URI_encode(V) (StringView_encode(V))
#define URI_decode(V) ((URI) StringView_decode(V))
#define DocumentUri_encode(V) (StringView_encode(V))
#define DocumentUri_decode(V) ((DocumentUri) StringView_decode(V))

JSON(StringView, StringView)
JSON(Null, Null)
JSON(Empty, Empty)

extern JSONValue         notification_encode(Notification *notification);
extern JSONValue         request_encode(Request *request);
extern bool              response_success(Response *response);
extern bool              response_error(Response *response);
extern Response          response_decode(JSONValue *json);

JSON(StringList, StringList)
OPTIONAL_JSON(StringView)
OPTIONAL_JSON(URI)
OPTIONAL_JSON(DocumentUri)
OPTIONAL_JSON(Bool)
DA_WITH_NAME(uint32_t, UInts);
JSON(UInts, UInts)

#endif /* __LSP_LSP_BASE_H__ */
