/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/CompletionItemLabelDetails.h>

DA_IMPL(CompletionItemLabelDetails)

OptionalJSONValue CompletionItemLabelDetailss_encode(CompletionItemLabelDetailss value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, CompletionItemLabelDetails_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalCompletionItemLabelDetailss CompletionItemLabelDetailss_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(CompletionItemLabelDetailss);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    CompletionItemLabelDetailss ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue                  elem = json_at(&json.value, ix);
        OptionalCompletionItemLabelDetails val = CompletionItemLabelDetails_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(CompletionItemLabelDetailss);
        }
        da_append_CompletionItemLabelDetails(&ret, val.value);
    }
    RETURN_VALUE(CompletionItemLabelDetailss, ret);
}

OptionalCompletionItemLabelDetails CompletionItemLabelDetails_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(CompletionItemLabelDetails);
    }
    CompletionItemLabelDetails value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "detail");
        value.detail = StringView_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "description");
        value.description = StringView_decode(v0);
    }
    RETURN_VALUE(CompletionItemLabelDetails, value);
}

OptionalJSONValue CompletionItemLabelDetails_encode(CompletionItemLabelDetails value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.detail.has_value) {
            _encoded_maybe = StringView_encode(value.detail.value);
        }
        json_optional_set(&v1, "detail", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.description.has_value) {
            _encoded_maybe = StringView_encode(value.description.value);
        }
        json_optional_set(&v1, "description", _encoded_maybe);
    }
    RETURN_VALUE(JSONValue, v1);
}
