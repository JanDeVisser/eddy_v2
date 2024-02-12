/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */
 
// clang-format off 

#ifndef __LSP_SYNCHRONIZATION_H__
#define __LSP_SYNCHRONIZATION_H__

#include <lsp/lsp_base.h>
#include <lsp/base.h>

typedef struct {
    TextDocumentItem textDocument;
} DidOpenTextDocumentParams;

extern OptionalJSONValue DidOpenTextDocumentParams_encode(DidOpenTextDocumentParams value);


// clang-format on
#endif /* __LSP_SYNCHRONIZATION_H__ */

