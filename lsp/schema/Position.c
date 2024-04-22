/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/Position.h>

DA_IMPL(Position)

OptionalJSONValue Positions_encode(Positions value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, Position_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalPositions Positions_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(Positions);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    Positions ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue elem = json_at(&json.value, ix);
        OptionalPosition  val = Position_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(Positions);
        }
        da_append_Position(&ret, val.value);
    }
    RETURN_VALUE(Positions, ret);
}

OptionalPosition Position_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(Position);
    }
    Position value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "line");
        value.line = FORWARD_OPTIONAL(UInt32, Position, UInt32_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "character");
        value.character = FORWARD_OPTIONAL(UInt32, Position, UInt32_decode(v0));
    }
    RETURN_VALUE(Position, value);
}

OptionalJSONValue Position_encode(Position value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = UInt32_encode(value.line);
        json_optional_set(&v1, "line", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = UInt32_encode(value.character);
        json_optional_set(&v1, "character", _encoded_maybe);
    }
    RETURN_VALUE(JSONValue, v1);
}
