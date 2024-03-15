/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/ClientCapabilities.h>

DA_IMPL(ClientCapabilities)

OptionalJSONValue OptionalClientCapabilities_encode(OptionalClientCapabilities value)
{
    if (value.has_value) {
        return ClientCapabilities_encode(value.value);
    } else {
        RETURN_EMPTY(JSONValue);
    }
}

OptionalOptionalClientCapabilities OptionalClientCapabilities_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(OptionalClientCapabilities);
    }
    RETURN_VALUE(OptionalClientCapabilities, ClientCapabilities_decode(json));
}

OptionalJSONValue ClientCapabilitiess_encode(ClientCapabilitiess value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, ClientCapabilities_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalClientCapabilitiess ClientCapabilitiess_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(ClientCapabilitiess);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    ClientCapabilitiess ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue          elem = json_at(&json.value, ix);
        OptionalClientCapabilities val = ClientCapabilities_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(ClientCapabilitiess);
        }
        da_append_ClientCapabilities(&ret, val.value);
    }
    RETURN_VALUE(ClientCapabilitiess, ret);
}
OptionalClientCapabilities ClientCapabilities_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(ClientCapabilities);
    }
    ClientCapabilities value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "textDocument");
        value.textDocument = FORWARD_OPTIONAL(OptionalTextDocumentClientCapabilities, ClientCapabilities, OptionalTextDocumentClientCapabilities_decode(v0));
    }
    RETURN_VALUE(ClientCapabilities, value);
}
OptionalJSONValue ClientCapabilities_encode(ClientCapabilities value)
{
    JSONValue v1 = json_object();
    json_optional_set(&v1, "textDocument", OptionalTextDocumentClientCapabilities_encode(value.textDocument));
    RETURN_VALUE(JSONValue, v1);
}
