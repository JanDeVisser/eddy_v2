/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/PositionEncodingKind.h>

DA_IMPL(PositionEncodingKind)

OptionalJSONValue OptionalPositionEncodingKind_encode(OptionalPositionEncodingKind value)
{
    if (value.has_value) {
        return PositionEncodingKind_encode(value.value);
    } else {
        RETURN_EMPTY(JSONValue);
    }
}

OptionalOptionalPositionEncodingKind OptionalPositionEncodingKind_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(OptionalPositionEncodingKind);
    }
    RETURN_VALUE(OptionalPositionEncodingKind, PositionEncodingKind_decode(json));
}

OptionalJSONValue PositionEncodingKinds_encode(PositionEncodingKinds value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, PositionEncodingKind_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalPositionEncodingKinds PositionEncodingKinds_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(PositionEncodingKinds);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    PositionEncodingKinds ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue            elem = json_at(&json.value, ix);
        OptionalPositionEncodingKind val = PositionEncodingKind_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(PositionEncodingKinds);
        }
        da_append_PositionEncodingKind(&ret, val.value);
    }
    RETURN_VALUE(PositionEncodingKinds, ret);
}
StringView PositionEncodingKind_to_string(PositionEncodingKind value)
{
    switch (value) {
    case PositionEncodingKindUTF8:
        return sv_from("utf-8");
    case PositionEncodingKindUTF16:
        return sv_from("utf-16");
    case PositionEncodingKindUTF32:
        return sv_from("utf-32");
    default:
        UNREACHABLE();
    }
}

OptionalPositionEncodingKind PositionEncodingKind_parse(StringView s)
{
    if (sv_eq_cstr(s, "utf-8"))
        RETURN_VALUE(PositionEncodingKind, PositionEncodingKindUTF8);
    if (sv_eq_cstr(s, "utf-16"))
        RETURN_VALUE(PositionEncodingKind, PositionEncodingKindUTF16);
    if (sv_eq_cstr(s, "utf-32"))
        RETURN_VALUE(PositionEncodingKind, PositionEncodingKindUTF32);
    RETURN_EMPTY(PositionEncodingKind);
}
OptionalPositionEncodingKind PositionEncodingKind_decode(OptionalJSONValue json)
{
    assert(json.has_value);
    assert(json.value.type == JSON_TYPE_STRING);
    return PositionEncodingKind_parse(json.value.string);
}
OptionalJSONValue PositionEncodingKind_encode(PositionEncodingKind value)
{
    RETURN_VALUE(JSONValue, json_string(PositionEncodingKind_to_string(value)));
}
