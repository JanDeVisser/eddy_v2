/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/PublishDiagnosticsParams.h>

DA_IMPL(PublishDiagnosticsParams)

OptionalJSONValue PublishDiagnosticsParamss_encode(PublishDiagnosticsParamss value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, PublishDiagnosticsParams_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalPublishDiagnosticsParamss PublishDiagnosticsParamss_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(PublishDiagnosticsParamss);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    PublishDiagnosticsParamss ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue                elem = json_at(&json.value, ix);
        OptionalPublishDiagnosticsParams val = PublishDiagnosticsParams_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(PublishDiagnosticsParamss);
        }
        da_append_PublishDiagnosticsParams(&ret, val.value);
    }
    RETURN_VALUE(PublishDiagnosticsParamss, ret);
}

OptionalPublishDiagnosticsParams PublishDiagnosticsParams_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(PublishDiagnosticsParams);
    }
    PublishDiagnosticsParams value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "uri");
        value.uri = FORWARD_OPTIONAL(DocumentUri, PublishDiagnosticsParams, DocumentUri_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "version");
        value.version = Int_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "diagnostics");
        value.diagnostics = FORWARD_OPTIONAL(Diagnostics, PublishDiagnosticsParams, Diagnostics_decode(v0));
    }
    RETURN_VALUE(PublishDiagnosticsParams, value);
}

OptionalJSONValue PublishDiagnosticsParams_encode(PublishDiagnosticsParams value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = DocumentUri_encode(value.uri);
        json_optional_set(&v1, "uri", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.version.has_value) {
            _encoded_maybe = Int_encode(value.version.value);
        }
        json_optional_set(&v1, "version", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = Diagnostics_encode(value.diagnostics);
        json_optional_set(&v1, "diagnostics", _encoded_maybe);
    }
    RETURN_VALUE(JSONValue, v1);
}
