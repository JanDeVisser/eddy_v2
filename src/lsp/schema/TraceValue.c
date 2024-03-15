/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/TraceValue.h>

DA_IMPL(TraceValue)

OptionalJSONValue OptionalTraceValue_encode(OptionalTraceValue value)
{
    if (value.has_value) {
        return TraceValue_encode(value.value);
    } else {
        RETURN_EMPTY(JSONValue);
    }
}

OptionalOptionalTraceValue OptionalTraceValue_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(OptionalTraceValue);
    }
    RETURN_VALUE(OptionalTraceValue, TraceValue_decode(json));
}

OptionalJSONValue TraceValues_encode(TraceValues value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, TraceValue_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalTraceValues TraceValues_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(TraceValues);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    TraceValues ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue  elem = json_at(&json.value, ix);
        OptionalTraceValue val = TraceValue_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(TraceValues);
        }
        da_append_TraceValue(&ret, val.value);
    }
    RETURN_VALUE(TraceValues, ret);
}
StringView TraceValue_to_string(TraceValue value)
{
    switch (value) {
    case TraceValueOff:
        return sv_from("off");
    case TraceValueMessages:
        return sv_from("messages");
    case TraceValueVerbose:
        return sv_from("verbose");
    default:
        UNREACHABLE();
    }
}

OptionalTraceValue TraceValue_parse(StringView s)
{
    if (sv_eq_cstr(s, "off"))
        RETURN_VALUE(TraceValue, TraceValueOff);
    if (sv_eq_cstr(s, "messages"))
        RETURN_VALUE(TraceValue, TraceValueMessages);
    if (sv_eq_cstr(s, "verbose"))
        RETURN_VALUE(TraceValue, TraceValueVerbose);
    RETURN_EMPTY(TraceValue);
}
OptionalTraceValue TraceValue_decode(OptionalJSONValue json)
{
    assert(json.has_value);
    assert(json.value.type == JSON_TYPE_STRING);
    return TraceValue_parse(json.value.string);
}
OptionalJSONValue TraceValue_encode(TraceValue value)
{
    RETURN_VALUE(JSONValue, json_string(TraceValue_to_string(value)));
}
