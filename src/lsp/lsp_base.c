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
DA_JSON_IMPL(StringView, StringList, strings)

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
    ret.id = json_get_int(json, "id", 0);
    ret.result = json_get(json, "result");
    ret.error = json_get(json, "error");
    assert(response_success(&ret) ^ response_error(&ret));
    return ret;
}
