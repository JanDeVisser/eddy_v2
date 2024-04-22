/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/DidCloseTextDocumentParams.h>

DA_IMPL(DidCloseTextDocumentParams)

OptionalJSONValue DidCloseTextDocumentParamss_encode(DidCloseTextDocumentParamss value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, DidCloseTextDocumentParams_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalDidCloseTextDocumentParamss DidCloseTextDocumentParamss_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(DidCloseTextDocumentParamss);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    DidCloseTextDocumentParamss ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue                  elem = json_at(&json.value, ix);
        OptionalDidCloseTextDocumentParams val = DidCloseTextDocumentParams_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(DidCloseTextDocumentParamss);
        }
        da_append_DidCloseTextDocumentParams(&ret, val.value);
    }
    RETURN_VALUE(DidCloseTextDocumentParamss, ret);
}

OptionalDidCloseTextDocumentParams DidCloseTextDocumentParams_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(DidCloseTextDocumentParams);
    }
    DidCloseTextDocumentParams value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "textDocument");
        value.textDocument = FORWARD_OPTIONAL(TextDocumentIdentifier, DidCloseTextDocumentParams, TextDocumentIdentifier_decode(v0));
    }
    RETURN_VALUE(DidCloseTextDocumentParams, value);
}

OptionalJSONValue DidCloseTextDocumentParams_encode(DidCloseTextDocumentParams value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = TextDocumentIdentifier_encode(value.textDocument);
        json_optional_set(&v1, "textDocument", _encoded_maybe);
    }
    RETURN_VALUE(JSONValue, v1);
}
