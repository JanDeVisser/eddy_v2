/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_DIDCLOSETEXTDOCUMENTPARAMS_H__
#define __LSP_DIDCLOSETEXTDOCUMENTPARAMS_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/TextDocumentIdentifier.h>

typedef struct {
    TextDocumentIdentifier textDocument;
} DidCloseTextDocumentParams;

OPTIONAL(DidCloseTextDocumentParams);
DA_WITH_NAME(DidCloseTextDocumentParams, DidCloseTextDocumentParamss);
OPTIONAL(DidCloseTextDocumentParamss);

extern OptionalJSONValue                   DidCloseTextDocumentParams_encode(DidCloseTextDocumentParams value);
extern OptionalDidCloseTextDocumentParams  DidCloseTextDocumentParams_decode(OptionalJSONValue json);
extern OptionalJSONValue                   DidCloseTextDocumentParamss_encode(DidCloseTextDocumentParamss value);
extern OptionalDidCloseTextDocumentParamss DidCloseTextDocumentParamss_decode(OptionalJSONValue json);

#endif /* __LSP_DIDCLOSETEXTDOCUMENTPARAMS_H__ */
