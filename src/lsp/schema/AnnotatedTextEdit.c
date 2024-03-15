/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/AnnotatedTextEdit.h>

DA_IMPL(AnnotatedTextEdit)

OptionalJSONValue OptionalAnnotatedTextEdit_encode(OptionalAnnotatedTextEdit value)
{
    if (value.has_value) {
        return AnnotatedTextEdit_encode(value.value);
    } else {
        RETURN_EMPTY(JSONValue);
    }
}

OptionalOptionalAnnotatedTextEdit OptionalAnnotatedTextEdit_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(OptionalAnnotatedTextEdit);
    }
    RETURN_VALUE(OptionalAnnotatedTextEdit, AnnotatedTextEdit_decode(json));
}

OptionalJSONValue AnnotatedTextEdits_encode(AnnotatedTextEdits value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, AnnotatedTextEdit_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalAnnotatedTextEdits AnnotatedTextEdits_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(AnnotatedTextEdits);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    AnnotatedTextEdits ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue         elem = json_at(&json.value, ix);
        OptionalAnnotatedTextEdit val = AnnotatedTextEdit_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(AnnotatedTextEdits);
        }
        da_append_AnnotatedTextEdit(&ret, val.value);
    }
    RETURN_VALUE(AnnotatedTextEdits, ret);
}
OptionalAnnotatedTextEdit AnnotatedTextEdit_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(AnnotatedTextEdit);
    }
    AnnotatedTextEdit value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "range");
        value.range = FORWARD_OPTIONAL(Range, AnnotatedTextEdit, Range_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "newText");
        value.newText = FORWARD_OPTIONAL(StringView, AnnotatedTextEdit, StringView_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "annotationId");
        value.annotationId = FORWARD_OPTIONAL(ChangeAnnotationIdentifier, AnnotatedTextEdit, ChangeAnnotationIdentifier_decode(v0));
    }
    RETURN_VALUE(AnnotatedTextEdit, value);
}
OptionalJSONValue AnnotatedTextEdit_encode(AnnotatedTextEdit value)
{
    JSONValue v1 = json_object();
    json_optional_set(&v1, "range", Range_encode(value.range));
    json_optional_set(&v1, "newText", StringView_encode(value.newText));
    json_optional_set(&v1, "annotationId", ChangeAnnotationIdentifier_encode(value.annotationId));
    RETURN_VALUE(JSONValue, v1);
}
