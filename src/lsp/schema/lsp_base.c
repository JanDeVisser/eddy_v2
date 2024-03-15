/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "json.h"
#include "sv.h"
#include <allocate.h>
#include <lsp/schema/lsp_base.h>

DECLARE_SHARED_ALLOCATOR(LSP)

// -- JSONValue -------------------------------------------------------------

OptionalJSONValue JSONValue_encode(JSONValue value)
{
    return OptionalJSONValue_create(value);
}

OptionalJSONValue JSONValue_decode(OptionalJSONValue value)
{
    return value;
}

// -- Int -------------------------------------------------------------------

OptionalJSONValue Int_encode(int value)
{
    RETURN_VALUE(JSONValue, json_int(value));
}

OptionalInt Int_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_INT) {
        RETURN_EMPTY(Int);
    }
    if (json.value.type != I32) {
        RETURN_EMPTY(Int);
    }
    RETURN_VALUE(Int, json.value.int_number.i32);
}

OptionalJSONValue Ints_encode(Ints value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, Int_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalInts Ints_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(Ints);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    Ints ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue elem = json_at(&json.value, ix);
        OptionalInt       val = Int_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(Ints);
        }
        da_append_int(&ret, val.value);
    }
    RETURN_VALUE(Ints, ret);
}

// -- UInt32 ----------------------------------------------------------------

OptionalJSONValue UInt32_encode(unsigned int value)
{
    RETURN_VALUE(JSONValue, json_integer((Integer) { .type = U32, .u32 = value }));
}

OptionalUInt32 UInt32_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(UInt32);
    }
    if (json.value.type != JSON_TYPE_INT) {
        RETURN_EMPTY(UInt32);
    }
    RETURN_VALUE(UInt32, json.value.int_number.u32);
}

OptionalJSONValue UInt32s_encode(UInt32s value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, UInt32_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalUInt32s UInt32s_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_ARRAY) {
        RETURN_EMPTY(UInt32s);
    }
    UInt32s ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue elem = json_at(&json.value, ix);
        OptionalUInt32    val = UInt32_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(UInt32s);
        }
        da_append_uint32_t(&ret, val.value);
    }
    RETURN_VALUE(UInt32s, ret);
}

// -- Bool ------------------------------------------------------------------

OptionalJSONValue Bool_encode(bool value)
{
    RETURN_VALUE(JSONValue, json_bool(value));
}

OptionalBool Bool_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_BOOLEAN) {
        RETURN_EMPTY(Bool);
    }
    RETURN_VALUE(Bool, json.value.boolean);
}

// -- StringView ------------------------------------------------------------

OptionalJSONValue StringView_encode(StringView sv)
{
    RETURN_VALUE(JSONValue, json_string(sv));
}

OptionalStringView StringView_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_STRING) {
        RETURN_EMPTY(StringView);
    }
    RETURN_VALUE(StringView, json.value.string);
}

OptionalJSONValue StringViews_encode(StringViews value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, StringView_encode(value.strings[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalStringViews StringViews_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(StringViews);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    StringViews ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue  elem = json_at(&json.value, ix);
        OptionalStringView val = StringView_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(StringViews);
        }
        da_append_StringView(&ret, val.value);
    }
    RETURN_VALUE(StringViews, ret);
}

// -- Empty -----------------------------------------------------------------

OptionalJSONValue Empty_encode(Empty value)
{
    RETURN_VALUE(JSONValue, json_object());
}

OptionalEmpty Empty_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT || json.value.object.size != 0) {
        RETURN_EMPTY(Empty);
    }
    RETURN_VALUE(Empty, (Empty) {});
}

// -- Null ------------------------------------------------------------------

OptionalJSONValue Null_encode(Null value)
{
    RETURN_VALUE(JSONValue, json_null());
}

OptionalNull Null_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_NULL) {
        RETURN_EMPTY(Null);
    }
    RETURN_VALUE(Null, (Null) {});
}

// ---------------------------------------------------------------------------

JSONValue notification_encode(Notification *notification)
{
    JSONValue ret = json_object();
    json_set_cstr(&ret, "jsonrpc", "2.0");
    json_set_cstr(&ret, "method", notification->method);
    if (notification->params.has_value) {
        json_set(&ret, "params", notification->params.value);
    }
    return ret;
}

JSONValue request_encode(Request *request)
{
    JSONValue ret = json_object();
    json_set_cstr(&ret, "jsonrpc", "2.0");
    json_set_int(&ret, "id", request->id);
    json_set_cstr(&ret, "method", request->method);
    if (request->params.has_value) {
        json_set(&ret, "params", request->params.value);
    }
    return ret;
}

bool response_success(Response *response)
{
    return response->result.has_value;
}

bool response_error(Response *response)
{
    return response->error.has_value;
}

Response response_decode(JSONValue *json)
{
    Response ret = { 0 };
    // trace(CAT_LSP, "response_decode():\n%.*s\n", SV_ARG(json_encode(*json)));
    ret.id = json_get_int(json, "id", 0);
    ret.result = json_get(json, "result");
    ret.error = json_get(json, "error");
    assert(response_success(&ret) ^ response_error(&ret));
    return ret;
}
