/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/MarkupKind.h>

DA_IMPL(MarkupKind)

OptionalJSONValue MarkupKinds_encode(MarkupKinds value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, MarkupKind_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalMarkupKinds MarkupKinds_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(MarkupKinds);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    MarkupKinds ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue  elem = json_at(&json.value, ix);
        OptionalMarkupKind val = MarkupKind_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(MarkupKinds);
        }
        da_append_MarkupKind(&ret, val.value);
    }
    RETURN_VALUE(MarkupKinds, ret);
}

StringView MarkupKind_to_string(MarkupKind value)
{
    switch (value) {
    case MarkupKindPlainText:
        return sv_from("plaintext");
    case MarkupKindMarkdown:
        return sv_from("markdown");
    default:
        UNREACHABLE();
    }
}

OptionalMarkupKind MarkupKind_parse(StringView s)
{
    if (sv_eq_cstr(s, "plaintext"))
        RETURN_VALUE(MarkupKind, MarkupKindPlainText);
    if (sv_eq_cstr(s, "markdown"))
        RETURN_VALUE(MarkupKind, MarkupKindMarkdown);
    RETURN_EMPTY(MarkupKind);
}

OptionalMarkupKind MarkupKind_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(MarkupKind);
    }
    assert(json.value.type == JSON_TYPE_STRING);
    return MarkupKind_parse(json.value.string);
}

OptionalJSONValue MarkupKind_encode(MarkupKind value)
{
    RETURN_VALUE(JSONValue, json_string(MarkupKind_to_string(value)));
}
