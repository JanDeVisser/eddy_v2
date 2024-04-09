/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/CodeDescription.h>

DA_IMPL(CodeDescription)

OptionalJSONValue CodeDescriptions_encode(CodeDescriptions value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, CodeDescription_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalCodeDescriptions CodeDescriptions_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(CodeDescriptions);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    CodeDescriptions ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue       elem = json_at(&json.value, ix);
        OptionalCodeDescription val = CodeDescription_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(CodeDescriptions);
        }
        da_append_CodeDescription(&ret, val.value);
    }
    RETURN_VALUE(CodeDescriptions, ret);
}

OptionalCodeDescription CodeDescription_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(CodeDescription);
    }
    CodeDescription value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "href");
        value.href = FORWARD_OPTIONAL(URI, CodeDescription, URI_decode(v0));
    }
    RETURN_VALUE(CodeDescription, value);
}

OptionalJSONValue CodeDescription_encode(CodeDescription value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = URI_encode(value.href);
        json_optional_set(&v1, "href", _encoded_maybe);
    }
    RETURN_VALUE(JSONValue, v1);
}
