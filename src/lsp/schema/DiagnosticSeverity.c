#include "json.h"
/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/DiagnosticSeverity.h>

DA_IMPL(DiagnosticSeverity)

OptionalJSONValue DiagnosticSeveritys_encode(DiagnosticSeveritys value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, DiagnosticSeverity_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalDiagnosticSeveritys DiagnosticSeveritys_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(DiagnosticSeveritys);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    DiagnosticSeveritys ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue          elem = json_at(&json.value, ix);
        OptionalDiagnosticSeverity val = DiagnosticSeverity_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(DiagnosticSeveritys);
        }
        da_append_DiagnosticSeverity(&ret, val.value);
    }
    RETURN_VALUE(DiagnosticSeveritys, ret);
}

OptionalDiagnosticSeverity DiagnosticSeverity_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(DiagnosticSeverity);
    }
    assert(json.value.type == JSON_TYPE_INT);
    if (1 == json_int_value(json.value))
        RETURN_VALUE(DiagnosticSeverity, DiagnosticSeverityError);
    if (2 == json_int_value(json.value))
        RETURN_VALUE(DiagnosticSeverity, DiagnosticSeverityWarning);
    if (3 == json_int_value(json.value))
        RETURN_VALUE(DiagnosticSeverity, DiagnosticSeverityInformation);
    if (4 == json_int_value(json.value))
        RETURN_VALUE(DiagnosticSeverity, DiagnosticSeverityHint);
    RETURN_EMPTY(DiagnosticSeverity);
}

OptionalJSONValue DiagnosticSeverity_encode(DiagnosticSeverity value)
{
    RETURN_VALUE(JSONValue, json_int(value));
}
