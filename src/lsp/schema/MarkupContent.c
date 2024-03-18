#include "json.h"
/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/MarkupContent.h>

DA_IMPL(MarkupContent)

OptionalJSONValue MarkupContents_encode(MarkupContents value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, MarkupContent_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalMarkupContents MarkupContents_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(MarkupContents);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    MarkupContents ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue     elem = json_at(&json.value, ix);
        OptionalMarkupContent val = MarkupContent_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(MarkupContents);
        }
        da_append_MarkupContent(&ret, val.value);
    }
    RETURN_VALUE(MarkupContents, ret);
}

OptionalMarkupContent MarkupContent_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(MarkupContent);
    }
    MarkupContent value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "kind");
        value.kind = FORWARD_OPTIONAL(MarkupKind, MarkupContent, MarkupKind_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "value");
        value.value = FORWARD_OPTIONAL(StringView, MarkupContent, StringView_decode(v0));
    }
    RETURN_VALUE(MarkupContent, value);
}

OptionalJSONValue MarkupContent_encode(MarkupContent value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = MarkupKind_encode(value.kind);
        json_optional_set(&v1, "kind", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = StringView_encode(value.value);
        json_optional_set(&v1, "value", _encoded_maybe);
    }
    RETURN_VALUE(JSONValue, v1);
}
