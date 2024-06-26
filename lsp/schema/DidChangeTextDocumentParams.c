/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/DidChangeTextDocumentParams.h>

DA_IMPL(DidChangeTextDocumentParams)

OptionalJSONValue DidChangeTextDocumentParamss_encode(DidChangeTextDocumentParamss value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, DidChangeTextDocumentParams_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalDidChangeTextDocumentParamss DidChangeTextDocumentParamss_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(DidChangeTextDocumentParamss);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    DidChangeTextDocumentParamss ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue                   elem = json_at(&json.value, ix);
        OptionalDidChangeTextDocumentParams val = DidChangeTextDocumentParams_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(DidChangeTextDocumentParamss);
        }
        da_append_DidChangeTextDocumentParams(&ret, val.value);
    }
    RETURN_VALUE(DidChangeTextDocumentParamss, ret);
}

OptionalDidChangeTextDocumentParams DidChangeTextDocumentParams_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(DidChangeTextDocumentParams);
    }
    DidChangeTextDocumentParams value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "textDocument");
        value.textDocument = FORWARD_OPTIONAL(VersionedTextDocumentIdentifier, DidChangeTextDocumentParams, VersionedTextDocumentIdentifier_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "contentChanges");
        value.contentChanges = FORWARD_OPTIONAL(TextDocumentContentChangeEvents, DidChangeTextDocumentParams, TextDocumentContentChangeEvents_decode(v0));
    }
    RETURN_VALUE(DidChangeTextDocumentParams, value);
}

OptionalJSONValue DidChangeTextDocumentParams_encode(DidChangeTextDocumentParams value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = VersionedTextDocumentIdentifier_encode(value.textDocument);
        json_optional_set(&v1, "textDocument", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = TextDocumentContentChangeEvents_encode(value.contentChanges);
        json_optional_set(&v1, "contentChanges", _encoded_maybe);
    }
    RETURN_VALUE(JSONValue, v1);
}
