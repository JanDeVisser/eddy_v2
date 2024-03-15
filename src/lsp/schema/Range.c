/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/Range.h>

DA_IMPL(Range)

OptionalJSONValue OptionalRange_encode(OptionalRange value)
{
    if (value.has_value) {
        return Range_encode(value.value);
    } else {
        RETURN_EMPTY(JSONValue);
    }
}

OptionalOptionalRange OptionalRange_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(OptionalRange);
    }
    RETURN_VALUE(OptionalRange, Range_decode(json));
}

OptionalJSONValue Ranges_encode(Ranges value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, Range_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalRanges Ranges_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(Ranges);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    Ranges ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue elem = json_at(&json.value, ix);
        OptionalRange     val = Range_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(Ranges);
        }
        da_append_Range(&ret, val.value);
    }
    RETURN_VALUE(Ranges, ret);
}
OptionalRange Range_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(Range);
    }
    Range value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "start");
        value.start = FORWARD_OPTIONAL(Position, Range, Position_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "end");
        value.end = FORWARD_OPTIONAL(Position, Range, Position_decode(v0));
    }
    RETURN_VALUE(Range, value);
}
OptionalJSONValue Range_encode(Range value)
{
    JSONValue v1 = json_object();
    json_optional_set(&v1, "start", Position_encode(value.start));
    json_optional_set(&v1, "end", Position_encode(value.end));
    RETURN_VALUE(JSONValue, v1);
}
