/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <raylib.h>

#define STATIC_ALLOCATOR
#include <allocate.h>
#include <command.h>

DA_IMPL(CommandParameter);

bool command_bind(Commands *commands, StringView name, int key)
{
    for (size_t ix = 0; ix < commands->bindings.size; ++ix) {
        if (key == commands->bindings.elements[ix].key) {
            return false;
        }
    }
    da_append_KeyBinding(&commands->bindings, (KeyBinding) { name, key });
    return true;
}

void command_register(Commands *commands, Command cmd, int key)
{
    StringView name = cmd.name;
    size_t ix;
    for (ix = 0; ix < commands->size; ++ix) {
        if (sv_eq(commands->elements[ix].name, name)) {
            commands->elements[ix] = cmd;
            break;
        }
    }
    if (ix == commands->size) {
        DIA_APPEND(Command, commands, cmd);
    }
    if (key != KEY_NULL) {
        command_bind(commands, name, key);
    }
}

bool command_is_key_bound(Commands *commands, int key)
{
    for (size_t ix = 0; ix < commands->bindings.size; ++ix) {
        if (key == commands->bindings.elements[ix].key) {
            return true;
        }
    }
    return false;
}

Command* command_for_key(Commands *commands, int key)
{
    for (size_t ix = 0; ix < commands->bindings.size; ++ix) {
        if (key == commands->bindings.elements[ix].key) {
            return command_get_command(commands, commands->bindings.elements[ix].command);
        }
    }
    return NULL;
}

Command* command_get_command(Commands *commands, StringView name)
{
    for (size_t ix = 0; ix < commands->size; ++ix) {
        if (sv_eq(commands->elements[ix].name, name)) {
            return commands->elements + ix;
        }
    }
    return NULL;
}
