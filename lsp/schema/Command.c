/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/Command.h>

DA_IMPL(Command)

OptionalJSONValue Commands_encode(Commands value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, Command_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalCommands Commands_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(Commands);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    Commands ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue elem = json_at(&json.value, ix);
        OptionalCommand   val = Command_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(Commands);
        }
        da_append_Command(&ret, val.value);
    }
    RETURN_VALUE(Commands, ret);
}

OptionalCommand Command_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(Command);
    }
    Command value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "title");
        value.title = FORWARD_OPTIONAL(StringView, Command, StringView_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "command");
        value.command = FORWARD_OPTIONAL(StringView, Command, StringView_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "arguments");
        value.arguments = JSONValues_decode(v0);
    }
    RETURN_VALUE(Command, value);
}

OptionalJSONValue Command_encode(Command value)
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
