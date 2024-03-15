#include "json.h"
/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/LocationLink.h>

DA_IMPL(LocationLink)

OptionalJSONValue LocationLinks_encode(LocationLinks value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, LocationLink_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalLocationLinks LocationLinks_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(LocationLinks);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    LocationLinks ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue    elem = json_at(&json.value, ix);
        OptionalLocationLink val = LocationLink_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(LocationLinks);
        }
        da_append_LocationLink(&ret, val.value);
    }
    RETURN_VALUE(LocationLinks, ret);
}

OptionalLocationLink LocationLink_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(LocationLink);
    }
    LocationLink value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "originSelectionRange");
        value.originSelectionRange = Range_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "targetUri");
        value.targetUri = FORWARD_OPTIONAL(DocumentUri, LocationLink, DocumentUri_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "targetRange");
        value.targetRange = FORWARD_OPTIONAL(Range, LocationLink, Range_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "targetSelectionRange");
        value.targetSelectionRange = FORWARD_OPTIONAL(Range, LocationLink, Range_decode(v0));
    }
    RETURN_VALUE(LocationLink, value);
}

OptionalJSONValue LocationLink_encode(LocationLink value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.originSelectionRange.has_value) {
            _encoded_maybe = Range_encode(value.originSelectionRange.value);
        }
        json_optional_set(&v1, "originSelectionRange", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = DocumentUri_encode(value.targetUri);
        json_optional_set(&v1, "targetUri", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = Range_encode(value.targetRange);
        json_optional_set(&v1, "targetRange", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = Range_encode(value.targetSelectionRange);
        json_optional_set(&v1, "targetSelectionRange", _encoded_maybe);
    }
    RETURN_VALUE(JSONValue, v1);
}
