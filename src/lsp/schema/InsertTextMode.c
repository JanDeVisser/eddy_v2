#include "json.h"
/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/InsertTextMode.h>

DA_IMPL(InsertTextMode)

OptionalJSONValue InsertTextModes_encode(InsertTextModes value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, InsertTextMode_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalInsertTextModes InsertTextModes_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(InsertTextModes);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    InsertTextModes ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue      elem = json_at(&json.value, ix);
        OptionalInsertTextMode val = InsertTextMode_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(InsertTextModes);
        }
        da_append_InsertTextMode(&ret, val.value);
    }
    RETURN_VALUE(InsertTextModes, ret);
}

OptionalInsertTextMode InsertTextMode_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(InsertTextMode);
    }
    assert(json.value.type == JSON_TYPE_INT);
    if (1 == json_int_value(json.value))
        RETURN_VALUE(InsertTextMode, InsertTextModeAsIs);
    if (2 == json_int_value(json.value))
        RETURN_VALUE(InsertTextMode, InsertTextModeAdjustIndentation);
    RETURN_EMPTY(InsertTextMode);
}

OptionalJSONValue InsertTextMode_encode(InsertTextMode value)
{
    RETURN_VALUE(JSONValue, json_int(value));
}
