/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/TextDocumentContentChangeEvent.h>

DA_IMPL(TextDocumentContentChangeEvent)

OptionalJSONValue OptionalTextDocumentContentChangeEvent_encode(OptionalTextDocumentContentChangeEvent value)
{
    if (value.has_value) {
        return TextDocumentContentChangeEvent_encode(value.value);
    } else {
        RETURN_EMPTY(JSONValue);
    }
}

OptionalOptionalTextDocumentContentChangeEvent OptionalTextDocumentContentChangeEvent_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(OptionalTextDocumentContentChangeEvent);
    }
    RETURN_VALUE(OptionalTextDocumentContentChangeEvent, TextDocumentContentChangeEvent_decode(json));
}

OptionalJSONValue TextDocumentContentChangeEvents_encode(TextDocumentContentChangeEvents value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, TextDocumentContentChangeEvent_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalTextDocumentContentChangeEvents TextDocumentContentChangeEvents_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(TextDocumentContentChangeEvents);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    TextDocumentContentChangeEvents ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue                      elem = json_at(&json.value, ix);
        OptionalTextDocumentContentChangeEvent val = TextDocumentContentChangeEvent_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(TextDocumentContentChangeEvents);
        }
        da_append_TextDocumentContentChangeEvent(&ret, val.value);
    }
    RETURN_VALUE(TextDocumentContentChangeEvents, ret);
}

OptionalTextDocumentContentChangeEvent TextDocumentContentChangeEvent_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(TextDocumentContentChangeEvent);
    }
    TextDocumentContentChangeEvent value = { 0 };
    assert(json.has_value);
    value.has_value = true;
    while (true) {
        {
            bool              decoded2 = false;
            OptionalJSONValue v3 = { 0 };
            do {
                v3 = json_get(&json.value, "range");
                OptionalRange opt2_0_range = Range_decode(v3);
                if (!opt2_0_range.has_value) {
                    break;
                }
                value._0.range = opt2_0_range.value;
                v3 = json_get(&json.value, "rangeLength");
                OptionalOptionalUInt32 opt2_0_rangeLength = OptionalUInt32_decode(v3);
                if (!opt2_0_rangeLength.has_value) {
                    break;
                }
                value._0.rangeLength = opt2_0_rangeLength.value;
                v3 = json_get(&json.value, "text");
                OptionalStringView opt2_0_text = StringView_decode(v3);
                if (!opt2_0_text.has_value) {
                    break;
                }
                value._0.text = opt2_0_text.value;
                decoded2 = true;
            } while (false);
            if (decoded2)
                break;
        }
        {
            bool              decoded2 = false;
            OptionalJSONValue v3 = { 0 };
            do {
                v3 = json_get(&json.value, "text");
                OptionalStringView opt2_1_text = StringView_decode(v3);
                if (!opt2_1_text.has_value) {
                    break;
                }
                value._1.text = opt2_1_text.value;
                decoded2 = true;
            } while (false);
            if (decoded2)
                break;
        }
        RETURN_EMPTY(TextDocumentContentChangeEvent);
    }
    RETURN_VALUE(TextDocumentContentChangeEvent, value);
}

OptionalJSONValue TextDocumentContentChangeEvent_encode(TextDocumentContentChangeEvent value)
{
    JSONValue v2 = { 0 };
    switch (value.tag) {
    case 0: {
        v2 = json_object();
        json_optional_set(&v2, "range", Range_encode(value._0.range));
        json_optional_set(&v2, "rangeLength", OptionalUInt32_encode(value._0.rangeLength));
        json_optional_set(&v2, "text", StringView_encode(value._0.text));
    } break;
    case 1: {
        v2 = json_object();
        json_optional_set(&v2, "text", StringView_encode(value._1.text));
    } break;
    default:
        UNREACHABLE();
    }

    RETURN_VALUE(JSONValue, v2);
}
