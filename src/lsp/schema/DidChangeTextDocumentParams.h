/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_DIDCHANGETEXTDOCUMENTPARAMS_H__
#define __LSP_DIDCHANGETEXTDOCUMENTPARAMS_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/TextDocumentContentChangeEvent.h>
#include <lsp/schema/VersionedTextDocumentIdentifier.h>

typedef struct {
    VersionedTextDocumentIdentifier textDocument;
    TextDocumentContentChangeEvents contentChanges;
} DidChangeTextDocumentParams;

OPTIONAL(DidChangeTextDocumentParams);
DA_WITH_NAME(DidChangeTextDocumentParams, DidChangeTextDocumentParamss);
OPTIONAL(DidChangeTextDocumentParamss);

extern OptionalJSONValue                    DidChangeTextDocumentParams_encode(DidChangeTextDocumentParams value);
extern OptionalDidChangeTextDocumentParams  DidChangeTextDocumentParams_decode(OptionalJSONValue json);
extern OptionalJSONValue                    DidChangeTextDocumentParamss_encode(DidChangeTextDocumentParamss value);
extern OptionalDidChangeTextDocumentParamss DidChangeTextDocumentParamss_decode(OptionalJSONValue json);

#endif /* __LSP_DIDCHANGETEXTDOCUMENTPARAMS_H__ */
