/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_TEXTDOCUMENTPOSITIONPARAMS_H__
#define __LSP_TEXTDOCUMENTPOSITIONPARAMS_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/Position.h>
#include <lsp/schema/TextDocumentIdentifier.h>

typedef struct {
    TextDocumentIdentifier textDocument;
    Position               position;
} TextDocumentPositionParams;

OPTIONAL(TextDocumentPositionParams);
DA_WITH_NAME(TextDocumentPositionParams, TextDocumentPositionParamss);
OPTIONAL(TextDocumentPositionParamss);

extern OptionalJSONValue                   TextDocumentPositionParams_encode(TextDocumentPositionParams value);
extern OptionalTextDocumentPositionParams  TextDocumentPositionParams_decode(OptionalJSONValue json);
extern OptionalJSONValue                   TextDocumentPositionParamss_encode(TextDocumentPositionParamss value);
extern OptionalTextDocumentPositionParamss TextDocumentPositionParamss_decode(OptionalJSONValue json);

#endif /* __LSP_TEXTDOCUMENTPOSITIONPARAMS_H__ */
