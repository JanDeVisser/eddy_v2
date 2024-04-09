/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/CompletionItemKind.h>

DA_IMPL(CompletionItemKind)

OptionalJSONValue CompletionItemKinds_encode(CompletionItemKinds value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, CompletionItemKind_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalCompletionItemKinds CompletionItemKinds_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(CompletionItemKinds);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    CompletionItemKinds ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue          elem = json_at(&json.value, ix);
        OptionalCompletionItemKind val = CompletionItemKind_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(CompletionItemKinds);
        }
        da_append_CompletionItemKind(&ret, val.value);
    }
    RETURN_VALUE(CompletionItemKinds, ret);
}

OptionalCompletionItemKind CompletionItemKind_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(CompletionItemKind);
    }
    assert(json.value.type == JSON_TYPE_INT);
    if (1 == json_int_value(json.value))
        RETURN_VALUE(CompletionItemKind, CompletionItemKindText);
    if (2 == json_int_value(json.value))
        RETURN_VALUE(CompletionItemKind, CompletionItemKindMethod);
    if (3 == json_int_value(json.value))
        RETURN_VALUE(CompletionItemKind, CompletionItemKindFunction);
    if (4 == json_int_value(json.value))
        RETURN_VALUE(CompletionItemKind, CompletionItemKindConstructor);
    if (5 == json_int_value(json.value))
        RETURN_VALUE(CompletionItemKind, CompletionItemKindField);
    if (6 == json_int_value(json.value))
        RETURN_VALUE(CompletionItemKind, CompletionItemKindVariable);
    if (7 == json_int_value(json.value))
        RETURN_VALUE(CompletionItemKind, CompletionItemKindClass);
    if (8 == json_int_value(json.value))
        RETURN_VALUE(CompletionItemKind, CompletionItemKindInterface);
    if (9 == json_int_value(json.value))
        RETURN_VALUE(CompletionItemKind, CompletionItemKindModule);
    if (10 == json_int_value(json.value))
        RETURN_VALUE(CompletionItemKind, CompletionItemKindProperty);
    if (11 == json_int_value(json.value))
        RETURN_VALUE(CompletionItemKind, CompletionItemKindUnit);
    if (12 == json_int_value(json.value))
        RETURN_VALUE(CompletionItemKind, CompletionItemKindValue);
    if (13 == json_int_value(json.value))
        RETURN_VALUE(CompletionItemKind, CompletionItemKindEnum);
    if (14 == json_int_value(json.value))
        RETURN_VALUE(CompletionItemKind, CompletionItemKindKeyword);
    if (15 == json_int_value(json.value))
        RETURN_VALUE(CompletionItemKind, CompletionItemKindSnippet);
    if (16 == json_int_value(json.value))
        RETURN_VALUE(CompletionItemKind, CompletionItemKindColor);
    if (17 == json_int_value(json.value))
        RETURN_VALUE(CompletionItemKind, CompletionItemKindFile);
    if (18 == json_int_value(json.value))
        RETURN_VALUE(CompletionItemKind, CompletionItemKindReference);
    if (19 == json_int_value(json.value))
        RETURN_VALUE(CompletionItemKind, CompletionItemKindFolder);
    if (20 == json_int_value(json.value))
        RETURN_VALUE(CompletionItemKind, CompletionItemKindEnumMember);
    if (21 == json_int_value(json.value))
        RETURN_VALUE(CompletionItemKind, CompletionItemKindConstant);
    if (22 == json_int_value(json.value))
        RETURN_VALUE(CompletionItemKind, CompletionItemKindStruct);
    if (23 == json_int_value(json.value))
        RETURN_VALUE(CompletionItemKind, CompletionItemKindEvent);
    if (24 == json_int_value(json.value))
        RETURN_VALUE(CompletionItemKind, CompletionItemKindOperator);
    if (25 == json_int_value(json.value))
        RETURN_VALUE(CompletionItemKind, CompletionItemKindTypeParameter);
    RETURN_EMPTY(CompletionItemKind);
}

OptionalJSONValue CompletionItemKind_encode(CompletionItemKind value)
{
    RETURN_VALUE(JSONValue, json_int(value));
}
