/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <allocate.h>
#include <lsp/lsp_base.h>

DECLARE_SHARED_ALLOCATOR(LSP)

OPTIONAL_JSON_IMPL(StringView)
OPTIONAL_JSON_IMPL(URI)
OPTIONAL_JSON_IMPL(DocumentUri)
OPTIONAL_JSON_IMPL(Bool)
OPTIONAL_JSON_IMPL(UInt)
DA_JSON_IMPL(StringView, StringList, strings)

OptionalJSONValue UInts_encode(UInts value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) { json_append(&ret, ((OptionalJSONValue){ .has_value = true, .value = json_int(value.elements[ix]) }).value); }
    return (OptionalJSONValue){ .has_value = true, .value = (ret) };
}

UInts UInts_decode(OptionalJSONValue json)
{
    if (!(json.has_value)) { _fatal("%s:%d: " "%s:%d: assert('%s') FAILED", "_file_name_short_", 15, "_file_name_short_", 15, "json.has_value"); };
    if (!(json.value.type == JSON_TYPE_ARRAY)) { _fatal("%s:%d: " "%s:%d: assert('%s') FAILED", "_file_name_short_", 15, "_file_name_short_", 15, "json.value.type == JSON_TYPE_ARRAY"); };
    UInts ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue elem = json_at(&json.value, ix);
        da_append_uint32_t(&ret, ( {
            if (!(elem.has_value)) { _fatal("%s:%d: " "%s:%d: assert('%s') FAILED", "_file_name_short_", 15, "_file_name_short_", 15, "elem.has_value"); };
            if (!(elem.value.type == JSON_TYPE_INT)) { _fatal("%s:%d: " "%s:%d: assert('%s') FAILED", "_file_name_short_", 15, "_file_name_short_", 15, "elem.value.type == JSON_TYPE_INT"); };
            (uint32_t) json_int_value(elem.value);
        }));
    }
    return ret;
}

OptionalJSONValue StringView_encode(StringView sv)
{
    return (OptionalJSONValue) { .has_value = true, .value = json_string(sv) };
}

StringView StringView_decode(OptionalJSONValue json)
{
    assert(json.has_value);
    assert(json.value.type == JSON_TYPE_STRING);
    return json.value.string;
}

OptionalJSONValue Empty_encode(Empty value)
{
    return (OptionalJSONValue) { .has_value = true, .value = json_object() };
}

Empty Empty_decode(OptionalJSONValue json)
{
    assert(json.has_value);
    assert(json.value.type == JSON_TYPE_OBJECT && json.value.object.size == 0);
    return (Empty) {};
}

OptionalJSONValue Null_encode(Null value)
{
    return (OptionalJSONValue) { .has_value = true, .value = json_null() };
}

Null Null_decode(OptionalJSONValue json)
{
    assert(json.has_value);
    assert(json.value.type == JSON_TYPE_NULL);
    return (Null) {};
}

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
