/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/TextDocumentIdentifier.h>

DA_IMPL(TextDocumentIdentifier)

OptionalJSONValue OptionalTextDocumentIdentifier_encode(OptionalTextDocumentIdentifier value)
{
    if (value.has_value) {
        return TextDocumentIdentifier_encode(value.value);
    } else {
        RETURN_EMPTY(JSONValue);
    }
}

OptionalOptionalTextDocumentIdentifier OptionalTextDocumentIdentifier_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(OptionalTextDocumentIdentifier);
    }
    RETURN_VALUE(OptionalTextDocumentIdentifier, TextDocumentIdentifier_decode(json));
}

OptionalJSONValue TextDocumentIdentifiers_encode(TextDocumentIdentifiers value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, TextDocumentIdentifier_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalTextDocumentIdentifiers TextDocumentIdentifiers_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(TextDocumentIdentifiers);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    TextDocumentIdentifiers ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue              elem = json_at(&json.value, ix);
        OptionalTextDocumentIdentifier val = TextDocumentIdentifier_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(TextDocumentIdentifiers);
        }
        da_append_TextDocumentIdentifier(&ret, val.value);
    }
    RETURN_VALUE(TextDocumentIdentifiers, ret);
}
OptionalTextDocumentIdentifier TextDocumentIdentifier_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(TextDocumentIdentifier);
    }
    TextDocumentIdentifier value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "uri");
        value.uri = FORWARD_OPTIONAL(DocumentUri, TextDocumentIdentifier, DocumentUri_decode(v0));
    }
    RETURN_VALUE(TextDocumentIdentifier, value);
}
OptionalJSONValue TextDocumentIdentifier_encode(TextDocumentIdentifier value)
{
    JSONValue v1 = json_object();
    json_optional_set(&v1, "uri", DocumentUri_encode(value.uri));
    RETURN_VALUE(JSONValue, v1);
}
