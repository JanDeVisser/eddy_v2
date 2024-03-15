/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/VersionedTextDocumentIdentifier.h>

DA_IMPL(VersionedTextDocumentIdentifier)

OptionalJSONValue OptionalVersionedTextDocumentIdentifier_encode(OptionalVersionedTextDocumentIdentifier value)
{
    if (value.has_value) {
        return VersionedTextDocumentIdentifier_encode(value.value);
    } else {
        RETURN_EMPTY(JSONValue);
    }
}

OptionalOptionalVersionedTextDocumentIdentifier OptionalVersionedTextDocumentIdentifier_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(OptionalVersionedTextDocumentIdentifier);
    }
    RETURN_VALUE(OptionalVersionedTextDocumentIdentifier, VersionedTextDocumentIdentifier_decode(json));
}

OptionalJSONValue VersionedTextDocumentIdentifiers_encode(VersionedTextDocumentIdentifiers value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, VersionedTextDocumentIdentifier_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalVersionedTextDocumentIdentifiers VersionedTextDocumentIdentifiers_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(VersionedTextDocumentIdentifiers);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    VersionedTextDocumentIdentifiers ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue                       elem = json_at(&json.value, ix);
        OptionalVersionedTextDocumentIdentifier val = VersionedTextDocumentIdentifier_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(VersionedTextDocumentIdentifiers);
        }
        da_append_VersionedTextDocumentIdentifier(&ret, val.value);
    }
    RETURN_VALUE(VersionedTextDocumentIdentifiers, ret);
}
OptionalVersionedTextDocumentIdentifier VersionedTextDocumentIdentifier_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(VersionedTextDocumentIdentifier);
    }
    VersionedTextDocumentIdentifier value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "uri");
        value.uri = FORWARD_OPTIONAL(DocumentUri, VersionedTextDocumentIdentifier, DocumentUri_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "version");
        value.version = FORWARD_OPTIONAL(Int, VersionedTextDocumentIdentifier, Int_decode(v0));
    }
    RETURN_VALUE(VersionedTextDocumentIdentifier, value);
}
OptionalJSONValue VersionedTextDocumentIdentifier_encode(VersionedTextDocumentIdentifier value)
{
    JSONValue v1 = json_object();
    json_optional_set(&v1, "uri", DocumentUri_encode(value.uri));
    json_optional_set(&v1, "version", Int_encode(value.version));
    RETURN_VALUE(JSONValue, v1);
}
