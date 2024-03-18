#include "json.h"
/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/CompletionTriggerKind.h>

DA_IMPL(CompletionTriggerKind)

OptionalJSONValue CompletionTriggerKinds_encode(CompletionTriggerKinds value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, CompletionTriggerKind_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalCompletionTriggerKinds CompletionTriggerKinds_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(CompletionTriggerKinds);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    CompletionTriggerKinds ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue             elem = json_at(&json.value, ix);
        OptionalCompletionTriggerKind val = CompletionTriggerKind_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(CompletionTriggerKinds);
        }
        da_append_CompletionTriggerKind(&ret, val.value);
    }
    RETURN_VALUE(CompletionTriggerKinds, ret);
}

OptionalCompletionTriggerKind CompletionTriggerKind_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(CompletionTriggerKind);
    }
    assert(json.value.type == JSON_TYPE_INT);
    if (1 == json_int_value(json.value))
        RETURN_VALUE(CompletionTriggerKind, CompletionTriggerKindInvoked);
    if (2 == json_int_value(json.value))
        RETURN_VALUE(CompletionTriggerKind, CompletionTriggerKindTriggerCharacter);
    if (3 == json_int_value(json.value))
        RETURN_VALUE(CompletionTriggerKind, CompletionTriggerKindTriggerForIncompleteCompletions);
    RETURN_EMPTY(CompletionTriggerKind);
}

OptionalJSONValue CompletionTriggerKind_encode(CompletionTriggerKind value)
{
    RETURN_VALUE(JSONValue, json_int(value));
}
