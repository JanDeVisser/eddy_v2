/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/TextDocumentPositionParams.h>

DA_IMPL(TextDocumentPositionParams)

OptionalJSONValue OptionalTextDocumentPositionParams_encode(OptionalTextDocumentPositionParams value)
{
    if (value.has_value) {
        return TextDocumentPositionParams_encode(value.value);
    } else {
        RETURN_EMPTY(JSONValue);
    }
}

OptionalOptionalTextDocumentPositionParams OptionalTextDocumentPositionParams_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(OptionalTextDocumentPositionParams);
    }
    RETURN_VALUE(OptionalTextDocumentPositionParams, TextDocumentPositionParams_decode(json));
}

OptionalJSONValue TextDocumentPositionParamss_encode(TextDocumentPositionParamss value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, TextDocumentPositionParams_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalTextDocumentPositionParamss TextDocumentPositionParamss_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(TextDocumentPositionParamss);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    TextDocumentPositionParamss ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue                  elem = json_at(&json.value, ix);
        OptionalTextDocumentPositionParams val = TextDocumentPositionParams_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(TextDocumentPositionParamss);
        }
        da_append_TextDocumentPositionParams(&ret, val.value);
    }
    RETURN_VALUE(TextDocumentPositionParamss, ret);
}
OptionalTextDocumentPositionParams TextDocumentPositionParams_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(TextDocumentPositionParams);
    }
    TextDocumentPositionParams value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "textDocument");
        value.textDocument = FORWARD_OPTIONAL(TextDocumentIdentifier, TextDocumentPositionParams, TextDocumentIdentifier_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "position");
        value.position = FORWARD_OPTIONAL(Position, TextDocumentPositionParams, Position_decode(v0));
    }
    RETURN_VALUE(TextDocumentPositionParams, value);
}
OptionalJSONValue TextDocumentPositionParams_encode(TextDocumentPositionParams value)
{
    JSONValue v1 = json_object();
    json_optional_set(&v1, "textDocument", TextDocumentIdentifier_encode(value.textDocument));
    json_optional_set(&v1, "position", Position_encode(value.position));
    RETURN_VALUE(JSONValue, v1);
}
