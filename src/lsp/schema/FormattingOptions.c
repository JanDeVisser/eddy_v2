#include "json.h"
/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/FormattingOptions.h>

DA_IMPL(FormattingOptions)

OptionalJSONValue FormattingOptionss_encode(FormattingOptionss value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, FormattingOptions_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalFormattingOptionss FormattingOptionss_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(FormattingOptionss);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    FormattingOptionss ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue         elem = json_at(&json.value, ix);
        OptionalFormattingOptions val = FormattingOptions_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(FormattingOptionss);
        }
        da_append_FormattingOptions(&ret, val.value);
    }
    RETURN_VALUE(FormattingOptionss, ret);
}

OptionalFormattingOptions FormattingOptions_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(FormattingOptions);
    }
    FormattingOptions value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "tabSize");
        value.tabSize = FORWARD_OPTIONAL(UInt32, FormattingOptions, UInt32_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "insertSpaces");
        value.insertSpaces = FORWARD_OPTIONAL(Bool, FormattingOptions, Bool_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "trimTrailingWhitespace");
        value.trimTrailingWhitespace = Bool_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "insertFinalNewline");
        value.insertFinalNewline = Bool_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "trimFinalNewlines");
        value.trimFinalNewlines = Bool_decode(v0);
    }
    RETURN_VALUE(FormattingOptions, value);
}

OptionalJSONValue FormattingOptions_encode(FormattingOptions value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = UInt32_encode(value.tabSize);
        json_optional_set(&v1, "tabSize", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = Bool_encode(value.insertSpaces);
        json_optional_set(&v1, "insertSpaces", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.trimTrailingWhitespace.has_value) {
            _encoded_maybe = Bool_encode(value.trimTrailingWhitespace.value);
        }
        json_optional_set(&v1, "trimTrailingWhitespace", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.insertFinalNewline.has_value) {
            _encoded_maybe = Bool_encode(value.insertFinalNewline.value);
        }
        json_optional_set(&v1, "insertFinalNewline", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.trimFinalNewlines.has_value) {
            _encoded_maybe = Bool_encode(value.trimFinalNewlines.value);
        }
        json_optional_set(&v1, "trimFinalNewlines", _encoded_maybe);
    }
    RETURN_VALUE(JSONValue, v1);
}
