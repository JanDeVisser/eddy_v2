#include "json.h"
/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/TextDocumentItem.h>

DA_IMPL(TextDocumentItem)

OptionalJSONValue TextDocumentItems_encode(TextDocumentItems value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, TextDocumentItem_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalTextDocumentItems TextDocumentItems_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(TextDocumentItems);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    TextDocumentItems ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue        elem = json_at(&json.value, ix);
        OptionalTextDocumentItem val = TextDocumentItem_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(TextDocumentItems);
        }
        da_append_TextDocumentItem(&ret, val.value);
    }
    RETURN_VALUE(TextDocumentItems, ret);
}

OptionalTextDocumentItem TextDocumentItem_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(TextDocumentItem);
    }
    TextDocumentItem value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "uri");
        value.uri = FORWARD_OPTIONAL(DocumentUri, TextDocumentItem, DocumentUri_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "languageId");
        value.languageId = FORWARD_OPTIONAL(StringView, TextDocumentItem, StringView_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "version");
        value.version = FORWARD_OPTIONAL(Int, TextDocumentItem, Int_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "text");
        value.text = FORWARD_OPTIONAL(StringView, TextDocumentItem, StringView_decode(v0));
    }
    RETURN_VALUE(TextDocumentItem, value);
}

OptionalJSONValue TextDocumentItem_encode(TextDocumentItem value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = DocumentUri_encode(value.uri);
        json_optional_set(&v1, "uri", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = StringView_encode(value.languageId);
        json_optional_set(&v1, "languageId", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = Int_encode(value.version);
        json_optional_set(&v1, "version", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = StringView_encode(value.text);
        json_optional_set(&v1, "text", _encoded_maybe);
    }
    RETURN_VALUE(JSONValue, v1);
}
