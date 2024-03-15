#include "json.h"
/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/TokenFormat.h>

DA_IMPL(TokenFormat)

OptionalJSONValue TokenFormats_encode(TokenFormats value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, TokenFormat_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalTokenFormats TokenFormats_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(TokenFormats);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    TokenFormats ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue   elem = json_at(&json.value, ix);
        OptionalTokenFormat val = TokenFormat_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(TokenFormats);
        }
        da_append_TokenFormat(&ret, val.value);
    }
    RETURN_VALUE(TokenFormats, ret);
}

StringView TokenFormat_to_string(TokenFormat value)
{
    switch (value) {
    case TokenFormatRelative:
        return sv_from("relative");
    default:
        UNREACHABLE();
    }
}

OptionalTokenFormat TokenFormat_parse(StringView s)
{
    if (sv_eq_cstr(s, "relative"))
        RETURN_VALUE(TokenFormat, TokenFormatRelative);
    RETURN_EMPTY(TokenFormat);
}

OptionalTokenFormat TokenFormat_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(TokenFormat);
    }
    assert(json.value.type == JSON_TYPE_STRING);
    return TokenFormat_parse(json.value.string);
}

OptionalJSONValue TokenFormat_encode(TokenFormat value)
{
    RETURN_VALUE(JSONValue, json_string(TokenFormat_to_string(value)));
}
