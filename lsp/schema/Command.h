/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_COMMAND_H__
#define __LSP_COMMAND_H__

#include <lsp/schema/lsp_base.h>

typedef struct {
    StringView         title;
    StringView         command;
    OptionalJSONValues arguments;
} Command;

OPTIONAL(Command);
DA_WITH_NAME(Command, Commands);
OPTIONAL(Commands);

extern OptionalJSONValue Command_encode(Command value);
extern OptionalCommand   Command_decode(OptionalJSONValue json);
extern OptionalJSONValue Commands_encode(Commands value);
extern OptionalCommands  Commands_decode(OptionalJSONValue json);

#endif /* __LSP_COMMAND_H__ */
