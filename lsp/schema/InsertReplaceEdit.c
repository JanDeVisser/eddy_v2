/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/InsertReplaceEdit.h>

DA_IMPL(InsertReplaceEdit)

OptionalJSONValue InsertReplaceEdits_encode(InsertReplaceEdits value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, InsertReplaceEdit_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalInsertReplaceEdits InsertReplaceEdits_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(InsertReplaceEdits);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    InsertReplaceEdits ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue         elem = json_at(&json.value, ix);
        OptionalInsertReplaceEdit val = InsertReplaceEdit_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(InsertReplaceEdits);
        }
        da_append_InsertReplaceEdit(&ret, val.value);
    }
    RETURN_VALUE(InsertReplaceEdits, ret);
}

OptionalInsertReplaceEdit InsertReplaceEdit_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(InsertReplaceEdit);
    }
    InsertReplaceEdit value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "newText");
        value.newText = FORWARD_OPTIONAL(StringView, InsertReplaceEdit, StringView_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "insert");
        value.insert = FORWARD_OPTIONAL(Range, InsertReplaceEdit, Range_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "replace");
        value.replace = FORWARD_OPTIONAL(Range, InsertReplaceEdit, Range_decode(v0));
    }
    RETURN_VALUE(InsertReplaceEdit, value);
}

OptionalJSONValue InsertReplaceEdit_encode(InsertReplaceEdit value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = StringView_encode(value.newText);
        json_optional_set(&v1, "newText", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = Range_encode(value.insert);
        json_optional_set(&v1, "insert", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = Range_encode(value.replace);
        json_optional_set(&v1, "replace", _encoded_maybe);
    }
    RETURN_VALUE(JSONValue, v1);
}
