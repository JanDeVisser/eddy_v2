/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/SemanticTokens.h>

DA_IMPL(SemanticTokens)

OptionalJSONValue SemanticTokenss_encode(SemanticTokenss value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, SemanticTokens_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalSemanticTokenss SemanticTokenss_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(SemanticTokenss);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    SemanticTokenss ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue      elem = json_at(&json.value, ix);
        OptionalSemanticTokens val = SemanticTokens_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(SemanticTokenss);
        }
        da_append_SemanticTokens(&ret, val.value);
    }
    RETURN_VALUE(SemanticTokenss, ret);
}

OptionalSemanticTokens SemanticTokens_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(SemanticTokens);
    }
    SemanticTokens value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "resultId");
        value.resultId = StringView_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "data");
        value.data = FORWARD_OPTIONAL(UInt32s, SemanticTokens, UInt32s_decode(v0));
    }
    RETURN_VALUE(SemanticTokens, value);
}

OptionalJSONValue SemanticTokens_encode(SemanticTokens value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.resultId.has_value) {
            _encoded_maybe = StringView_encode(value.resultId.value);
        }
        json_optional_set(&v1, "resultId", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = UInt32s_encode(value.data);
        json_optional_set(&v1, "data", _encoded_maybe);
    }
    RETURN_VALUE(JSONValue, v1);
}
