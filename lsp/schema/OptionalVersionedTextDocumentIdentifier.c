/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/OptionalVersionedTextDocumentIdentifier.h>

DA_IMPL(OptionalVersionedTextDocumentIdentifier)

OptionalJSONValue OptionalVersionedTextDocumentIdentifiers_encode(OptionalVersionedTextDocumentIdentifiers value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, OptionalVersionedTextDocumentIdentifier_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalOptionalVersionedTextDocumentIdentifiers OptionalVersionedTextDocumentIdentifiers_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(OptionalVersionedTextDocumentIdentifiers);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    OptionalVersionedTextDocumentIdentifiers ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue                               elem = json_at(&json.value, ix);
        OptionalOptionalVersionedTextDocumentIdentifier val = OptionalVersionedTextDocumentIdentifier_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(OptionalVersionedTextDocumentIdentifiers);
        }
        da_append_OptionalVersionedTextDocumentIdentifier(&ret, val.value);
    }
    RETURN_VALUE(OptionalVersionedTextDocumentIdentifiers, ret);
}

OptionalOptionalVersionedTextDocumentIdentifier OptionalVersionedTextDocumentIdentifier_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(OptionalVersionedTextDocumentIdentifier);
    }
    OptionalVersionedTextDocumentIdentifier value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "uri");
        value.uri = FORWARD_OPTIONAL(DocumentUri, OptionalVersionedTextDocumentIdentifier, DocumentUri_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "version");
        assert(v0.has_value);
        value.version.has_value = true;
        while (true) {
            {
                OptionalInt decoded = Int_decode(v0);
                if (decoded.has_value) {
                    value.version.tag = 0;
                    value.version._0 = decoded.value;
                    break;
                }
            }
            {
                OptionalNull decoded = Null_decode(v0);
                if (decoded.has_value) {
                    value.version.tag = 1;
                    value.version._1 = decoded.value;
                    break;
                }
            }
            RETURN_EMPTY(OptionalVersionedTextDocumentIdentifier);
        }
    }
    RETURN_VALUE(OptionalVersionedTextDocumentIdentifier, value);
}

OptionalJSONValue OptionalVersionedTextDocumentIdentifier_encode(OptionalVersionedTextDocumentIdentifier value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = DocumentUri_encode(value.uri);
        json_optional_set(&v1, "uri", _encoded_maybe);
    }
    assert(value.version.has_value);
    {
        JSONValue v2 = { 0 };
        switch (value.version.tag) {
        case 0:
            v2 = MUST_OPTIONAL(JSONValue, Int_encode(value.version._0));

            break;
        case 1:
            v2 = MUST_OPTIONAL(JSONValue, Null_encode(value.version._1));

            break;
        default:
            UNREACHABLE();
        }

        json_set(&v1, "version", v2);
    }

    RETURN_VALUE(JSONValue, v1);
}
