/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/LSPCommand.h>

DA_IMPL(LSPCommand)

OptionalJSONValue LSPCommands_encode(LSPCommands value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, LSPCommand_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalLSPCommands LSPCommands_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(LSPCommands);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    LSPCommands ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue  elem = json_at(&json.value, ix);
        OptionalLSPCommand val = LSPCommand_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(LSPCommands);
        }
        da_append_LSPCommand(&ret, val.value);
    }
    RETURN_VALUE(LSPCommands, ret);
}

OptionalLSPCommand LSPCommand_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(LSPCommand);
    }
    LSPCommand value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "title");
        value.title = FORWARD_OPTIONAL(StringView, LSPCommand, StringView_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "command");
        value.command = FORWARD_OPTIONAL(StringView, LSPCommand, StringView_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "arguments");
        value.arguments = JSONValues_decode(v0);
    }
    RETURN_VALUE(LSPCommand, value);
}

OptionalJSONValue LSPCommand_encode(LSPCommand value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = StringView_encode(value.title);
        json_optional_set(&v1, "title", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = StringView_encode(value.command);
        json_optional_set(&v1, "command", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.arguments.has_value) {
            _encoded_maybe = JSONValues_encode(value.arguments.value);
        }
        json_optional_set(&v1, "arguments", _encoded_maybe);
    }
    RETURN_VALUE(JSONValue, v1);
}
