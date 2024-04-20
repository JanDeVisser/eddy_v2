/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/Location.h>

DA_IMPL(Location)

OptionalJSONValue Locations_encode(Locations value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, Location_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalLocations Locations_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(Locations);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    Locations ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue elem = json_at(&json.value, ix);
        OptionalLocation  val = Location_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(Locations);
        }
        da_append_Location(&ret, val.value);
    }
    RETURN_VALUE(Locations, ret);
}

OptionalLocation Location_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(Location);
    }
    Location value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "uri");
        value.uri = FORWARD_OPTIONAL(DocumentUri, Location, DocumentUri_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "range");
        value.range = FORWARD_OPTIONAL(Range, Location, Range_decode(v0));
    }
    RETURN_VALUE(Location, value);
}

OptionalJSONValue Location_encode(Location value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = DocumentUri_encode(value.uri);
        json_optional_set(&v1, "uri", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = Range_encode(value.range);
        json_optional_set(&v1, "range", _encoded_maybe);
    }
    RETURN_VALUE(JSONValue, v1);
}
