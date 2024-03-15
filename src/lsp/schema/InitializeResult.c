#include "json.h"
/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/InitializeResult.h>

DA_IMPL(InitializeResult)

OptionalJSONValue InitializeResults_encode(InitializeResults value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, InitializeResult_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalInitializeResults InitializeResults_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(InitializeResults);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    InitializeResults ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue        elem = json_at(&json.value, ix);
        OptionalInitializeResult val = InitializeResult_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(InitializeResults);
        }
        da_append_InitializeResult(&ret, val.value);
    }
    RETURN_VALUE(InitializeResults, ret);
}

OptionalInitializeResult InitializeResult_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(InitializeResult);
    }
    InitializeResult value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "capabilities");
        value.capabilities = FORWARD_OPTIONAL(ServerCapabilities, InitializeResult, ServerCapabilities_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "serverInfo");
        if (v0.has_value) {
            value.serverInfo.has_value = true;
            {
                OptionalJSONValue v1 = json_get(&v0.value, "name");
                value.serverInfo.name = FORWARD_OPTIONAL(StringView, InitializeResult, StringView_decode(v1));
            }

            {
                OptionalJSONValue v1 = json_get(&v0.value, "version");
                value.serverInfo.version = StringView_decode(v1);
            }
        }
    }
    RETURN_VALUE(InitializeResult, value);
}

OptionalJSONValue InitializeResult_encode(InitializeResult value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = ServerCapabilities_encode(value.capabilities);
        json_optional_set(&v1, "capabilities", _encoded_maybe);
    }
    if (value.serverInfo.has_value) {
        JSONValue v2 = json_object();
        {
            OptionalJSONValue _encoded_maybe = { 0 };
            _encoded_maybe = StringView_encode(value.serverInfo.name);
            json_optional_set(&v2, "name", _encoded_maybe);
        }
        {
            OptionalJSONValue _encoded_maybe = { 0 };
            if (value.serverInfo.version.has_value) {
                _encoded_maybe = StringView_encode(value.serverInfo.version.value);
            }
            json_optional_set(&v2, "version", _encoded_maybe);
        }
        json_set(&v1, "serverInfo", v2);
    }
    RETURN_VALUE(JSONValue, v1);
}
