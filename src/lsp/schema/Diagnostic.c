#include "json.h"
/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/Diagnostic.h>

DA_IMPL(Diagnostic)

OptionalJSONValue Diagnostics_encode(Diagnostics value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, Diagnostic_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalDiagnostics Diagnostics_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(Diagnostics);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    Diagnostics ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue  elem = json_at(&json.value, ix);
        OptionalDiagnostic val = Diagnostic_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(Diagnostics);
        }
        da_append_Diagnostic(&ret, val.value);
    }
    RETURN_VALUE(Diagnostics, ret);
}

OptionalDiagnostic Diagnostic_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(Diagnostic);
    }
    Diagnostic value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "range");
        value.range = FORWARD_OPTIONAL(Range, Diagnostic, Range_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "severity");
        value.severity = DiagnosticSeverity_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "code");
        if (v0.has_value) {
            value.code.has_value = true;
            while (true) {
                {
                    OptionalInt decoded = Int_decode(v0);
                    if (decoded.has_value) {
                        value.code.tag = 0;
                        value.code._0 = decoded.value;
                        break;
                    }
                }
                {
                    OptionalStringView decoded = StringView_decode(v0);
                    if (decoded.has_value) {
                        value.code.tag = 1;
                        value.code._1 = decoded.value;
                        break;
                    }
                }
                RETURN_EMPTY(Diagnostic);
            }
        }
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "codeDescription");
        value.codeDescription = CodeDescription_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "source");
        value.source = StringView_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "message");
        value.message = FORWARD_OPTIONAL(StringView, Diagnostic, StringView_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "tags");
        value.tags = DiagnosticTags_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "relatedInformation");
        value.relatedInformation = DiagnosticRelatedInformations_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "data");
        value.data = JSONValue_decode(v0);
    }
    RETURN_VALUE(Diagnostic, value);
}

OptionalJSONValue Diagnostic_encode(Diagnostic value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = Range_encode(value.range);
        json_optional_set(&v1, "range", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.severity.has_value) {
            _encoded_maybe = DiagnosticSeverity_encode(value.severity.value);
        }
        json_optional_set(&v1, "severity", _encoded_maybe);
    }
    if (value.code.has_value) {
        JSONValue v2 = { 0 };
        switch (value.code.tag) {
        case 0:
            v2 = MUST_OPTIONAL(JSONValue, Int_encode(value.code._0));

            break;
        case 1:
            v2 = MUST_OPTIONAL(JSONValue, StringView_encode(value.code._1));

            break;
        default:
            UNREACHABLE();
        }

        json_set(&v1, "code", v2);
    }

    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.codeDescription.has_value) {
            _encoded_maybe = CodeDescription_encode(value.codeDescription.value);
        }
        json_optional_set(&v1, "codeDescription", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.source.has_value) {
            _encoded_maybe = StringView_encode(value.source.value);
        }
        json_optional_set(&v1, "source", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = StringView_encode(value.message);
        json_optional_set(&v1, "message", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.tags.has_value) {
            _encoded_maybe = DiagnosticTags_encode(value.tags.value);
        }
        json_optional_set(&v1, "tags", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.relatedInformation.has_value) {
            _encoded_maybe = DiagnosticRelatedInformations_encode(value.relatedInformation.value);
        }
        json_optional_set(&v1, "relatedInformation", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.data.has_value) {
            _encoded_maybe = JSONValue_encode(value.data.value);
        }
        json_optional_set(&v1, "data", _encoded_maybe);
    }
    RETURN_VALUE(JSONValue, v1);
}
