/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/PublishDiagnosticsClientCapabilities.h>

DA_IMPL(PublishDiagnosticsClientCapabilities)

OptionalJSONValue PublishDiagnosticsClientCapabilitiess_encode(PublishDiagnosticsClientCapabilitiess value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, PublishDiagnosticsClientCapabilities_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalPublishDiagnosticsClientCapabilitiess PublishDiagnosticsClientCapabilitiess_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(PublishDiagnosticsClientCapabilitiess);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    PublishDiagnosticsClientCapabilitiess ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue                            elem = json_at(&json.value, ix);
        OptionalPublishDiagnosticsClientCapabilities val = PublishDiagnosticsClientCapabilities_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(PublishDiagnosticsClientCapabilitiess);
        }
        da_append_PublishDiagnosticsClientCapabilities(&ret, val.value);
    }
    RETURN_VALUE(PublishDiagnosticsClientCapabilitiess, ret);
}

OptionalPublishDiagnosticsClientCapabilities PublishDiagnosticsClientCapabilities_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(PublishDiagnosticsClientCapabilities);
    }
    PublishDiagnosticsClientCapabilities value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "relatedInformation");
        value.relatedInformation = Bool_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "tagSupport");
        if (v0.has_value) {
            value.tagSupport.has_value = true;
            {
                OptionalJSONValue v1 = json_get(&v0.value, "valueSet");
                value.tagSupport.valueSet = FORWARD_OPTIONAL(DiagnosticTags, PublishDiagnosticsClientCapabilities, DiagnosticTags_decode(v1));
            }
        }
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "versionSupport");
        value.versionSupport = Bool_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "codeDescriptionSupport");
        value.codeDescriptionSupport = Bool_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "dataSupport");
        value.dataSupport = Bool_decode(v0);
    }
    RETURN_VALUE(PublishDiagnosticsClientCapabilities, value);
}

OptionalJSONValue PublishDiagnosticsClientCapabilities_encode(PublishDiagnosticsClientCapabilities value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.relatedInformation.has_value) {
            _encoded_maybe = Bool_encode(value.relatedInformation.value);
        }
        json_optional_set(&v1, "relatedInformation", _encoded_maybe);
    }
    if (value.tagSupport.has_value) {
        JSONValue v2 = json_object();
        {
            OptionalJSONValue _encoded_maybe = { 0 };
            _encoded_maybe = DiagnosticTags_encode(value.tagSupport.valueSet);
            json_optional_set(&v2, "valueSet", _encoded_maybe);
        }
        json_set(&v1, "tagSupport", v2);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.versionSupport.has_value) {
            _encoded_maybe = Bool_encode(value.versionSupport.value);
        }
        json_optional_set(&v1, "versionSupport", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.codeDescriptionSupport.has_value) {
            _encoded_maybe = Bool_encode(value.codeDescriptionSupport.value);
        }
        json_optional_set(&v1, "codeDescriptionSupport", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.dataSupport.has_value) {
            _encoded_maybe = Bool_encode(value.dataSupport.value);
        }
        json_optional_set(&v1, "dataSupport", _encoded_maybe);
    }
    RETURN_VALUE(JSONValue, v1);
}
