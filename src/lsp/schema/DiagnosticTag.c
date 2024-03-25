#include "json.h"
/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/DiagnosticTag.h>

DA_IMPL(DiagnosticTag)

OptionalJSONValue DiagnosticTags_encode(DiagnosticTags value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, DiagnosticTag_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalDiagnosticTags DiagnosticTags_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(DiagnosticTags);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    DiagnosticTags ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue     elem = json_at(&json.value, ix);
        OptionalDiagnosticTag val = DiagnosticTag_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(DiagnosticTags);
        }
        da_append_DiagnosticTag(&ret, val.value);
    }
    RETURN_VALUE(DiagnosticTags, ret);
}

OptionalDiagnosticTag DiagnosticTag_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(DiagnosticTag);
    }
    assert(json.value.type == JSON_TYPE_INT);
    if (1 == json_int_value(json.value))
        RETURN_VALUE(DiagnosticTag, DiagnosticTagUnnecessary);
    if (2 == json_int_value(json.value))
        RETURN_VALUE(DiagnosticTag, DiagnosticTagDeprecated);
    RETURN_EMPTY(DiagnosticTag);
}

OptionalJSONValue DiagnosticTag_encode(DiagnosticTag value)
{
    RETURN_VALUE(JSONValue, json_int(value));
}
