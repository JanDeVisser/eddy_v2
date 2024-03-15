/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/DidOpenTextDocumentParams.h>

DA_IMPL(DidOpenTextDocumentParams)

OptionalJSONValue OptionalDidOpenTextDocumentParams_encode(OptionalDidOpenTextDocumentParams value)
{
    if (value.has_value) {
        return DidOpenTextDocumentParams_encode(value.value);
    } else {
        RETURN_EMPTY(JSONValue);
    }
}

OptionalOptionalDidOpenTextDocumentParams OptionalDidOpenTextDocumentParams_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(OptionalDidOpenTextDocumentParams);
    }
    RETURN_VALUE(OptionalDidOpenTextDocumentParams, DidOpenTextDocumentParams_decode(json));
}

OptionalJSONValue DidOpenTextDocumentParamss_encode(DidOpenTextDocumentParamss value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, DidOpenTextDocumentParams_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalDidOpenTextDocumentParamss DidOpenTextDocumentParamss_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(DidOpenTextDocumentParamss);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    DidOpenTextDocumentParamss ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue                 elem = json_at(&json.value, ix);
        OptionalDidOpenTextDocumentParams val = DidOpenTextDocumentParams_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(DidOpenTextDocumentParamss);
        }
        da_append_DidOpenTextDocumentParams(&ret, val.value);
    }
    RETURN_VALUE(DidOpenTextDocumentParamss, ret);
}
OptionalDidOpenTextDocumentParams DidOpenTextDocumentParams_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(DidOpenTextDocumentParams);
    }
    DidOpenTextDocumentParams value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "textDocument");
        value.textDocument = FORWARD_OPTIONAL(TextDocumentItem, DidOpenTextDocumentParams, TextDocumentItem_decode(v0));
    }
    RETURN_VALUE(DidOpenTextDocumentParams, value);
}
OptionalJSONValue DidOpenTextDocumentParams_encode(DidOpenTextDocumentParams value)
{
    JSONValue v1 = json_object();
    json_optional_set(&v1, "textDocument", TextDocumentItem_encode(value.textDocument));
    RETURN_VALUE(JSONValue, v1);
}
