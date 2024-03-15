/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/TextDocumentClientCapabilities.h>

DA_IMPL(TextDocumentClientCapabilities)

OptionalJSONValue OptionalTextDocumentClientCapabilities_encode(OptionalTextDocumentClientCapabilities value)
{
    if (value.has_value) {
        return TextDocumentClientCapabilities_encode(value.value);
    } else {
        RETURN_EMPTY(JSONValue);
    }
}

OptionalOptionalTextDocumentClientCapabilities OptionalTextDocumentClientCapabilities_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(OptionalTextDocumentClientCapabilities);
    }
    RETURN_VALUE(OptionalTextDocumentClientCapabilities, TextDocumentClientCapabilities_decode(json));
}

OptionalJSONValue TextDocumentClientCapabilitiess_encode(TextDocumentClientCapabilitiess value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, TextDocumentClientCapabilities_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalTextDocumentClientCapabilitiess TextDocumentClientCapabilitiess_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(TextDocumentClientCapabilitiess);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    TextDocumentClientCapabilitiess ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue                      elem = json_at(&json.value, ix);
        OptionalTextDocumentClientCapabilities val = TextDocumentClientCapabilities_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(TextDocumentClientCapabilitiess);
        }
        da_append_TextDocumentClientCapabilities(&ret, val.value);
    }
    RETURN_VALUE(TextDocumentClientCapabilitiess, ret);
}
OptionalTextDocumentClientCapabilities TextDocumentClientCapabilities_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(TextDocumentClientCapabilities);
    }
    TextDocumentClientCapabilities value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "synchronization");
        value.synchronization = FORWARD_OPTIONAL(OptionalTextDocumentSyncClientCapabilities, TextDocumentClientCapabilities, OptionalTextDocumentSyncClientCapabilities_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "semanticTokens");
        value.semanticTokens = FORWARD_OPTIONAL(OptionalSemanticTokensClientCapabilities, TextDocumentClientCapabilities, OptionalSemanticTokensClientCapabilities_decode(v0));
    }
    RETURN_VALUE(TextDocumentClientCapabilities, value);
}
OptionalJSONValue TextDocumentClientCapabilities_encode(TextDocumentClientCapabilities value)
{
    JSONValue v1 = json_object();
    json_optional_set(&v1, "synchronization", OptionalTextDocumentSyncClientCapabilities_encode(value.synchronization));
    json_optional_set(&v1, "semanticTokens", OptionalSemanticTokensClientCapabilities_encode(value.semanticTokens));
    RETURN_VALUE(JSONValue, v1);
}
