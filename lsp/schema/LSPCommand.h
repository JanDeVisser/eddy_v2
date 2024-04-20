/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_LSPCOMMAND_H__
#define __LSP_LSPCOMMAND_H__

#include <lsp/schema/lsp_base.h>

typedef struct {
    StringView         title;
    StringView         command;
    OptionalJSONValues arguments;
} LSPCommand;

OPTIONAL(LSPCommand);
DA_WITH_NAME(LSPCommand, LSPCommands);
OPTIONAL(LSPCommands);

extern OptionalJSONValue   LSPCommand_encode(LSPCommand value);
extern OptionalLSPCommand  LSPCommand_decode(OptionalJSONValue json);
extern OptionalJSONValue   LSPCommands_encode(LSPCommands value);
extern OptionalLSPCommands LSPCommands_decode(OptionalJSONValue json);

#endif /* __LSP_LSPCOMMAND_H__ */
