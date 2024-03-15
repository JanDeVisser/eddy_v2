/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/DidSaveTextDocumentParams.h>

DA_IMPL(DidSaveTextDocumentParams)

OptionalJSONValue OptionalDidSaveTextDocumentParams_encode(OptionalDidSaveTextDocumentParams value)
{
    if (value.has_value) {
        return DidSaveTextDocumentParams_encode(value.value);
    } else {
        RETURN_EMPTY(JSONValue);
    }
}

OptionalOptionalDidSaveTextDocumentParams OptionalDidSaveTextDocumentParams_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(OptionalDidSaveTextDocumentParams);
    }
    RETURN_VALUE(OptionalDidSaveTextDocumentParams, DidSaveTextDocumentParams_decode(json));
}

OptionalJSONValue DidSaveTextDocumentParamss_encode(DidSaveTextDocumentParamss value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, DidSaveTextDocumentParams_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalDidSaveTextDocumentParamss DidSaveTextDocumentParamss_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(DidSaveTextDocumentParamss);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    DidSaveTextDocumentParamss ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue                 elem = json_at(&json.value, ix);
        OptionalDidSaveTextDocumentParams val = DidSaveTextDocumentParams_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(DidSaveTextDocumentParamss);
        }
        da_append_DidSaveTextDocumentParams(&ret, val.value);
    }
    RETURN_VALUE(DidSaveTextDocumentParamss, ret);
}
OptionalDidSaveTextDocumentParams DidSaveTextDocumentParams_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(DidSaveTextDocumentParams);
    }
    DidSaveTextDocumentParams value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "textDocument");
        value.textDocument = FORWARD_OPTIONAL(TextDocumentIdentifier, DidSaveTextDocumentParams, TextDocumentIdentifier_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "text");
        value.text = FORWARD_OPTIONAL(OptionalStringView, DidSaveTextDocumentParams, OptionalStringView_decode(v0));
    }
    RETURN_VALUE(DidSaveTextDocumentParams, value);
}
OptionalJSONValue DidSaveTextDocumentParams_encode(DidSaveTextDocumentParams value)
{
    JSONValue v1 = json_object();
    json_optional_set(&v1, "textDocument", TextDocumentIdentifier_encode(value.textDocument));
    json_optional_set(&v1, "text", OptionalStringView_encode(value.text));
    RETURN_VALUE(JSONValue, v1);
}
