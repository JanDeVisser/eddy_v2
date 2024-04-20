/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/DiagnosticRelatedInformation.h>

DA_IMPL(DiagnosticRelatedInformation)

OptionalJSONValue DiagnosticRelatedInformations_encode(DiagnosticRelatedInformations value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, DiagnosticRelatedInformation_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalDiagnosticRelatedInformations DiagnosticRelatedInformations_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(DiagnosticRelatedInformations);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    DiagnosticRelatedInformations ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue                    elem = json_at(&json.value, ix);
        OptionalDiagnosticRelatedInformation val = DiagnosticRelatedInformation_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(DiagnosticRelatedInformations);
        }
        da_append_DiagnosticRelatedInformation(&ret, val.value);
    }
    RETURN_VALUE(DiagnosticRelatedInformations, ret);
}

OptionalDiagnosticRelatedInformation DiagnosticRelatedInformation_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(DiagnosticRelatedInformation);
    }
    DiagnosticRelatedInformation value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "location");
        value.location = FORWARD_OPTIONAL(Location, DiagnosticRelatedInformation, Location_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "message");
        value.message = FORWARD_OPTIONAL(StringView, DiagnosticRelatedInformation, StringView_decode(v0));
    }
    RETURN_VALUE(DiagnosticRelatedInformation, value);
}

OptionalJSONValue DiagnosticRelatedInformation_encode(DiagnosticRelatedInformation value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = Location_encode(value.location);
        json_optional_set(&v1, "location", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = StringView_encode(value.message);
        json_optional_set(&v1, "message", _encoded_maybe);
    }
    RETURN_VALUE(JSONValue, v1);
}
