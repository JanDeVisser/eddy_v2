/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_DIDOPENTEXTDOCUMENTPARAMS_H__
#define __LSP_DIDOPENTEXTDOCUMENTPARAMS_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/TextDocumentItem.h>

typedef struct {
    TextDocumentItem textDocument;
} DidOpenTextDocumentParams;

OPTIONAL(DidOpenTextDocumentParams);
DA_WITH_NAME(DidOpenTextDocumentParams, DidOpenTextDocumentParamss);
OPTIONAL(DidOpenTextDocumentParamss);

extern OptionalJSONValue                  DidOpenTextDocumentParams_encode(DidOpenTextDocumentParams value);
extern OptionalDidOpenTextDocumentParams  DidOpenTextDocumentParams_decode(OptionalJSONValue json);
extern OptionalJSONValue                  DidOpenTextDocumentParamss_encode(DidOpenTextDocumentParamss value);
extern OptionalDidOpenTextDocumentParamss DidOpenTextDocumentParamss_decode(OptionalJSONValue json);

#endif /* __LSP_DIDOPENTEXTDOCUMENTPARAMS_H__ */
