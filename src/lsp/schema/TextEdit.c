#include "json.h"
/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/TextEdit.h>

DA_IMPL(TextEdit)

OptionalJSONValue TextEdits_encode(TextEdits value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, TextEdit_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalTextEdits TextEdits_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(TextEdits);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    TextEdits ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue elem = json_at(&json.value, ix);
        OptionalTextEdit  val = TextEdit_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(TextEdits);
        }
        da_append_TextEdit(&ret, val.value);
    }
    RETURN_VALUE(TextEdits, ret);
}

OptionalTextEdit TextEdit_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(TextEdit);
    }
    TextEdit value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "range");
        value.range = FORWARD_OPTIONAL(Range, TextEdit, Range_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "newText");
        value.newText = FORWARD_OPTIONAL(StringView, TextEdit, StringView_decode(v0));
    }
    RETURN_VALUE(TextEdit, value);
}

OptionalJSONValue TextEdit_encode(TextEdit value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = Range_encode(value.range);
        json_optional_set(&v1, "range", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = StringView_encode(value.newText);
        json_optional_set(&v1, "newText", _encoded_maybe);
    }
    RETURN_VALUE(JSONValue, v1);
}
