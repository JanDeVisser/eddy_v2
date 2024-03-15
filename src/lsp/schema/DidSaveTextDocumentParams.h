/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_DIDSAVETEXTDOCUMENTPARAMS_H__
#define __LSP_DIDSAVETEXTDOCUMENTPARAMS_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/TextDocumentIdentifier.h>

typedef struct {
    TextDocumentIdentifier textDocument;
    OptionalStringView     text;
} DidSaveTextDocumentParams;

OPTIONAL(DidSaveTextDocumentParams);
OPTIONAL(OptionalDidSaveTextDocumentParams);
DA_WITH_NAME(DidSaveTextDocumentParams, DidSaveTextDocumentParamss);
OPTIONAL(DidSaveTextDocumentParamss);
OPTIONAL(OptionalDidSaveTextDocumentParamss);

extern OptionalJSONValue                         DidSaveTextDocumentParams_encode(DidSaveTextDocumentParams value);
extern OptionalDidSaveTextDocumentParams         DidSaveTextDocumentParams_decode(OptionalJSONValue json);
extern OptionalJSONValue                         OptionalDidSaveTextDocumentParams_encode(OptionalDidSaveTextDocumentParams value);
extern OptionalOptionalDidSaveTextDocumentParams OptionalDidSaveTextDocumentParams_decode(OptionalJSONValue json);
extern OptionalJSONValue                         DidSaveTextDocumentParamss_encode(DidSaveTextDocumentParamss value);
extern OptionalDidSaveTextDocumentParamss        DidSaveTextDocumentParamss_decode(OptionalJSONValue json);

#endif /* __LSP_DIDSAVETEXTDOCUMENTPARAMS_H__ */
