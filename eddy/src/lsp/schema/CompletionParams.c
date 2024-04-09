/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/CompletionParams.h>

DA_IMPL(CompletionParams)

OptionalJSONValue CompletionParamss_encode(CompletionParamss value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, CompletionParams_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalCompletionParamss CompletionParamss_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(CompletionParamss);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    CompletionParamss ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue        elem = json_at(&json.value, ix);
        OptionalCompletionParams val = CompletionParams_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(CompletionParamss);
        }
        da_append_CompletionParams(&ret, val.value);
    }
    RETURN_VALUE(CompletionParamss, ret);
}

OptionalCompletionParams CompletionParams_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(CompletionParams);
    }
    CompletionParams value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "textDocument");
        value.textDocument = FORWARD_OPTIONAL(TextDocumentIdentifier, CompletionParams, TextDocumentIdentifier_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "position");
        value.position = FORWARD_OPTIONAL(Position, CompletionParams, Position_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "context");
        value.context = CompletionContext_decode(v0);
    }
    RETURN_VALUE(CompletionParams, value);
}

OptionalJSONValue CompletionParams_encode(CompletionParams value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = TextDocumentIdentifier_encode(value.textDocument);
        json_optional_set(&v1, "textDocument", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = Position_encode(value.position);
        json_optional_set(&v1, "position", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.context.has_value) {
            _encoded_maybe = CompletionContext_encode(value.context.value);
        }
        json_optional_set(&v1, "context", _encoded_maybe);
    }
    RETURN_VALUE(JSONValue, v1);
}
