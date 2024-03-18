#include "json.h"
/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/CompletionContext.h>

DA_IMPL(CompletionContext)

OptionalJSONValue CompletionContexts_encode(CompletionContexts value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, CompletionContext_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalCompletionContexts CompletionContexts_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(CompletionContexts);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    CompletionContexts ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue         elem = json_at(&json.value, ix);
        OptionalCompletionContext val = CompletionContext_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(CompletionContexts);
        }
        da_append_CompletionContext(&ret, val.value);
    }
    RETURN_VALUE(CompletionContexts, ret);
}

OptionalCompletionContext CompletionContext_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(CompletionContext);
    }
    CompletionContext value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "triggerKind");
        value.triggerKind = FORWARD_OPTIONAL(CompletionTriggerKind, CompletionContext, CompletionTriggerKind_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "triggerCharacter");
        value.triggerCharacter = StringView_decode(v0);
    }
    RETURN_VALUE(CompletionContext, value);
}

OptionalJSONValue CompletionContext_encode(CompletionContext value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = CompletionTriggerKind_encode(value.triggerKind);
        json_optional_set(&v1, "triggerKind", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.triggerCharacter.has_value) {
            _encoded_maybe = StringView_encode(value.triggerCharacter.value);
        }
        json_optional_set(&v1, "triggerCharacter", _encoded_maybe);
    }
    RETURN_VALUE(JSONValue, v1);
}
