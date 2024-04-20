/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/SaveOptions.h>

DA_IMPL(SaveOptions)

OptionalJSONValue SaveOptionss_encode(SaveOptionss value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, SaveOptions_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalSaveOptionss SaveOptionss_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(SaveOptionss);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    SaveOptionss ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue   elem = json_at(&json.value, ix);
        OptionalSaveOptions val = SaveOptions_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(SaveOptionss);
        }
        da_append_SaveOptions(&ret, val.value);
    }
    RETURN_VALUE(SaveOptionss, ret);
}

OptionalSaveOptions SaveOptions_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(SaveOptions);
    }
    SaveOptions value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "includeText");
        value.includeText = Bool_decode(v0);
    }
    RETURN_VALUE(SaveOptions, value);
}

OptionalJSONValue SaveOptions_encode(SaveOptions value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.includeText.has_value) {
            _encoded_maybe = Bool_encode(value.includeText.value);
        }
        json_optional_set(&v1, "includeText", _encoded_maybe);
    }
    RETURN_VALUE(JSONValue, v1);
}
