/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/ChangeAnnotation.h>

DA_IMPL(ChangeAnnotation)

OptionalJSONValue ChangeAnnotations_encode(ChangeAnnotations value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, ChangeAnnotation_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalChangeAnnotations ChangeAnnotations_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(ChangeAnnotations);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    ChangeAnnotations ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue        elem = json_at(&json.value, ix);
        OptionalChangeAnnotation val = ChangeAnnotation_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(ChangeAnnotations);
        }
        da_append_ChangeAnnotation(&ret, val.value);
    }
    RETURN_VALUE(ChangeAnnotations, ret);
}

OptionalChangeAnnotation ChangeAnnotation_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(ChangeAnnotation);
    }
    ChangeAnnotation value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "label");
        value.label = FORWARD_OPTIONAL(StringView, ChangeAnnotation, StringView_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "needsConfirmation");
        value.needsConfirmation = Bool_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "description");
        value.description = StringView_decode(v0);
    }
    RETURN_VALUE(ChangeAnnotation, value);
}

OptionalJSONValue ChangeAnnotation_encode(ChangeAnnotation value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = StringView_encode(value.label);
        json_optional_set(&v1, "label", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.needsConfirmation.has_value) {
            _encoded_maybe = Bool_encode(value.needsConfirmation.value);
        }
        json_optional_set(&v1, "needsConfirmation", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.description.has_value) {
            _encoded_maybe = StringView_encode(value.description.value);
        }
        json_optional_set(&v1, "description", _encoded_maybe);
    }
    RETURN_VALUE(JSONValue, v1);
}
