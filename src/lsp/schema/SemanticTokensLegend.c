/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/SemanticTokensLegend.h>

DA_IMPL(SemanticTokensLegend)

OptionalJSONValue OptionalSemanticTokensLegend_encode(OptionalSemanticTokensLegend value)
{
    if (value.has_value) {
        return SemanticTokensLegend_encode(value.value);
    } else {
        RETURN_EMPTY(JSONValue);
    }
}

OptionalOptionalSemanticTokensLegend OptionalSemanticTokensLegend_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(OptionalSemanticTokensLegend);
    }
    RETURN_VALUE(OptionalSemanticTokensLegend, SemanticTokensLegend_decode(json));
}

OptionalJSONValue SemanticTokensLegends_encode(SemanticTokensLegends value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, SemanticTokensLegend_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalSemanticTokensLegends SemanticTokensLegends_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(SemanticTokensLegends);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    SemanticTokensLegends ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue            elem = json_at(&json.value, ix);
        OptionalSemanticTokensLegend val = SemanticTokensLegend_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(SemanticTokensLegends);
        }
        da_append_SemanticTokensLegend(&ret, val.value);
    }
    RETURN_VALUE(SemanticTokensLegends, ret);
}
OptionalSemanticTokensLegend SemanticTokensLegend_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(SemanticTokensLegend);
    }
    SemanticTokensLegend value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "tokenTypes");
        value.tokenTypes = FORWARD_OPTIONAL(StringViews, SemanticTokensLegend, StringViews_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "tokenModifiers");
        value.tokenModifiers = FORWARD_OPTIONAL(StringViews, SemanticTokensLegend, StringViews_decode(v0));
    }
    RETURN_VALUE(SemanticTokensLegend, value);
}
OptionalJSONValue SemanticTokensLegend_encode(SemanticTokensLegend value)
{
    JSONValue v1 = json_object();
    json_optional_set(&v1, "tokenTypes", StringViews_encode(value.tokenTypes));
    json_optional_set(&v1, "tokenModifiers", StringViews_encode(value.tokenModifiers));
    RETURN_VALUE(JSONValue, v1);
}
