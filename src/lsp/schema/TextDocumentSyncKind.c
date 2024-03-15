/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/TextDocumentSyncKind.h>

DA_IMPL(TextDocumentSyncKind)

OptionalJSONValue OptionalTextDocumentSyncKind_encode(OptionalTextDocumentSyncKind value)
{
    if (value.has_value) {
        return TextDocumentSyncKind_encode(value.value);
    } else {
        RETURN_EMPTY(JSONValue);
    }
}

OptionalOptionalTextDocumentSyncKind OptionalTextDocumentSyncKind_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(OptionalTextDocumentSyncKind);
    }
    RETURN_VALUE(OptionalTextDocumentSyncKind, TextDocumentSyncKind_decode(json));
}

OptionalJSONValue TextDocumentSyncKinds_encode(TextDocumentSyncKinds value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, TextDocumentSyncKind_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalTextDocumentSyncKinds TextDocumentSyncKinds_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(TextDocumentSyncKinds);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    TextDocumentSyncKinds ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue            elem = json_at(&json.value, ix);
        OptionalTextDocumentSyncKind val = TextDocumentSyncKind_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(TextDocumentSyncKinds);
        }
        da_append_TextDocumentSyncKind(&ret, val.value);
    }
    RETURN_VALUE(TextDocumentSyncKinds, ret);
}

OptionalTextDocumentSyncKind TextDocumentSyncKind_decode(OptionalJSONValue json)
{
    assert(json.has_value);
    assert(json.value.type == JSON_TYPE_INT);
    if (0 == json_int_value(json.value))
        RETURN_VALUE(TextDocumentSyncKind, TextDocumentSyncKindNone);
    if (1 == json_int_value(json.value))
        RETURN_VALUE(TextDocumentSyncKind, TextDocumentSyncKindFull);
    if (2 == json_int_value(json.value))
        RETURN_VALUE(TextDocumentSyncKind, TextDocumentSyncKindIncremental);
    RETURN_EMPTY(TextDocumentSyncKind);
}
OptionalJSONValue TextDocumentSyncKind_encode(TextDocumentSyncKind value)
{
    RETURN_VALUE(JSONValue, json_int(value));
}
