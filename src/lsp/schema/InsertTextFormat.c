#include "json.h"
/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/InsertTextFormat.h>

DA_IMPL(InsertTextFormat)

OptionalJSONValue InsertTextFormats_encode(InsertTextFormats value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, InsertTextFormat_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalInsertTextFormats InsertTextFormats_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(InsertTextFormats);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    InsertTextFormats ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue        elem = json_at(&json.value, ix);
        OptionalInsertTextFormat val = InsertTextFormat_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(InsertTextFormats);
        }
        da_append_InsertTextFormat(&ret, val.value);
    }
    RETURN_VALUE(InsertTextFormats, ret);
}

OptionalInsertTextFormat InsertTextFormat_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(InsertTextFormat);
    }
    assert(json.value.type == JSON_TYPE_INT);
    if (1 == json_int_value(json.value))
        RETURN_VALUE(InsertTextFormat, InsertTextFormatPlainText);
    if (2 == json_int_value(json.value))
        RETURN_VALUE(InsertTextFormat, InsertTextFormatSnippet);
    RETURN_EMPTY(InsertTextFormat);
}

OptionalJSONValue InsertTextFormat_encode(InsertTextFormat value)
{
    RETURN_VALUE(JSONValue, json_int(value));
}
