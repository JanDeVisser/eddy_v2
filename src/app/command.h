/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <sv.h>

#define COMMANDPARAMETER_TYPES(S) \
    S(BUFFER)                     \
    S(COMMAND)                    \
    S(DIRECTORY)                  \
    S(EXISTING_DIRECTORY)         \
    S(EXISTING_FILE_NAME)         \
    S(FILE_NAME)                  \
    S(INTEGER)                    \
    S(NEW_DIRECTORY)              \
    S(NEW_FILE_NAME)              \
    S(STRING)

typedef enum {
#undef COMMANDPARAMETER_TYPE
#define COMMANDPARAMETER_TYPE(T) CPT_##T,
    COMMANDPARAMETER_TYPES(COMMANDPARAMETER_TYPE)
#undef COMMANDPARAMETER_TYPE
} CommandParameterType;

typedef StringView (*GetDefault)(void);

typedef struct {
    StringView           prompt;
    CommandParameterType type;
    GetDefault           get_default;
} CommandParameter;

DA_WITH_NAME(CommandParameter, CommandParameters);

typedef void (*CommandHandler)(void *target, StringList parameters);

typedef struct {
    StringView        name;
    StringView        synopsis;
    CommandParameters parameters;
    CommandHandler    handler;
} Command;

typedef struct {
    StringView command;
    int        key;
} KeyBinding;

DA_WITH_NAME(KeyBinding, KeyBindings);

typedef struct {
    void   *owner;
    Command command;
} ScheduledCommand;

typedef struct {
                DIA(Command);
    KeyBindings bindings;
} Commands;

extern bool     command_bind(Commands *commands, StringView name, int key);
extern void     command_register(Commands *commands, Command cmd, int key);
extern bool     command_is_key_bound(Commands *commands, int key);
extern Command *command_for_key(Commands *commands, int key);
extern Command *command_get_command(Commands *commands, StringView cmd_name);

#endif /* __COMMAND_H__ */
