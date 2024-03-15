/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_DOCUMENTFORMATTINGPARAMS_H__
#define __LSP_DOCUMENTFORMATTINGPARAMS_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/FormattingOptions.h>
#include <lsp/schema/TextDocumentIdentifier.h>
#include <lsp/schema/WorkDoneProgressParams.h>

typedef struct {
    TextDocumentIdentifier textDocument;
    FormattingOptions      options;
} DocumentFormattingParams;

OPTIONAL(DocumentFormattingParams);
DA_WITH_NAME(DocumentFormattingParams, DocumentFormattingParamss);
OPTIONAL(DocumentFormattingParamss);

extern OptionalJSONValue                 DocumentFormattingParams_encode(DocumentFormattingParams value);
extern OptionalDocumentFormattingParams  DocumentFormattingParams_decode(OptionalJSONValue json);
extern OptionalJSONValue                 DocumentFormattingParamss_encode(DocumentFormattingParamss value);
extern OptionalDocumentFormattingParamss DocumentFormattingParamss_decode(OptionalJSONValue json);

#endif /* __LSP_DOCUMENTFORMATTINGPARAMS_H__ */
