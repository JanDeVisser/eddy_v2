/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/RegularExpressionsClientCapabilities.h>

DA_IMPL(RegularExpressionsClientCapabilities)

OptionalJSONValue OptionalRegularExpressionsClientCapabilities_encode(OptionalRegularExpressionsClientCapabilities value)
{
    if (value.has_value) {
        return RegularExpressionsClientCapabilities_encode(value.value);
    } else {
        RETURN_EMPTY(JSONValue);
    }
}

OptionalOptionalRegularExpressionsClientCapabilities OptionalRegularExpressionsClientCapabilities_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(OptionalRegularExpressionsClientCapabilities);
    }
    RETURN_VALUE(OptionalRegularExpressionsClientCapabilities, RegularExpressionsClientCapabilities_decode(json));
}

OptionalJSONValue RegularExpressionsClientCapabilitiess_encode(RegularExpressionsClientCapabilitiess value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, RegularExpressionsClientCapabilities_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalRegularExpressionsClientCapabilitiess RegularExpressionsClientCapabilitiess_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(RegularExpressionsClientCapabilitiess);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    RegularExpressionsClientCapabilitiess ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue                            elem = json_at(&json.value, ix);
        OptionalRegularExpressionsClientCapabilities val = RegularExpressionsClientCapabilities_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(RegularExpressionsClientCapabilitiess);
        }
        da_append_RegularExpressionsClientCapabilities(&ret, val.value);
    }
    RETURN_VALUE(RegularExpressionsClientCapabilitiess, ret);
}
OptionalRegularExpressionsClientCapabilities RegularExpressionsClientCapabilities_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(RegularExpressionsClientCapabilities);
    }
    RegularExpressionsClientCapabilities value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "engine");
        value.engine = FORWARD_OPTIONAL(StringView, RegularExpressionsClientCapabilities, StringView_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "version");
        value.version = FORWARD_OPTIONAL(OptionalStringView, RegularExpressionsClientCapabilities, OptionalStringView_decode(v0));
    }
    RETURN_VALUE(RegularExpressionsClientCapabilities, value);
}
OptionalJSONValue RegularExpressionsClientCapabilities_encode(RegularExpressionsClientCapabilities value)
{
    JSONValue v1 = json_object();
    json_optional_set(&v1, "engine", StringView_encode(value.engine));
    json_optional_set(&v1, "version", OptionalStringView_encode(value.version));
    RETURN_VALUE(JSONValue, v1);
}
