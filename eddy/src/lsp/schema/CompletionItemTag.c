/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/CompletionItemTag.h>

DA_IMPL(CompletionItemTag)

OptionalJSONValue CompletionItemTags_encode(CompletionItemTags value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, CompletionItemTag_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalCompletionItemTags CompletionItemTags_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(CompletionItemTags);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    CompletionItemTags ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue         elem = json_at(&json.value, ix);
        OptionalCompletionItemTag val = CompletionItemTag_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(CompletionItemTags);
        }
        da_append_CompletionItemTag(&ret, val.value);
    }
    RETURN_VALUE(CompletionItemTags, ret);
}

OptionalCompletionItemTag CompletionItemTag_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(CompletionItemTag);
    }
    assert(json.value.type == JSON_TYPE_INT);
    if (1 == json_int_value(json.value))
        RETURN_VALUE(CompletionItemTag, CompletionItemTagDeprecated);
    RETURN_EMPTY(CompletionItemTag);
}

OptionalJSONValue CompletionItemTag_encode(CompletionItemTag value)
{
    RETURN_VALUE(JSONValue, json_int(value));
}
