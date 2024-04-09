/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_INITIALIZERESULT_H__
#define __LSP_INITIALIZERESULT_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/ServerCapabilities.h>

typedef struct {
    ServerCapabilities capabilities;
    struct {
        bool               has_value;
        StringView         name;
        OptionalStringView version;
    } serverInfo;
} InitializeResult;

OPTIONAL(InitializeResult);
DA_WITH_NAME(InitializeResult, InitializeResults);
OPTIONAL(InitializeResults);

extern OptionalJSONValue         InitializeResult_encode(InitializeResult value);
extern OptionalInitializeResult  InitializeResult_decode(OptionalJSONValue json);
extern OptionalJSONValue         InitializeResults_encode(InitializeResults value);
extern OptionalInitializeResults InitializeResults_decode(OptionalJSONValue json);

#endif /* __LSP_INITIALIZERESULT_H__ */
