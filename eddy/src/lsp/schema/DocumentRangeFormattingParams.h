/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_DOCUMENTRANGEFORMATTINGPARAMS_H__
#define __LSP_DOCUMENTRANGEFORMATTINGPARAMS_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/FormattingOptions.h>
#include <lsp/schema/Range.h>
#include <lsp/schema/TextDocumentIdentifier.h>
#include <lsp/schema/WorkDoneProgressParams.h>

typedef struct {
    TextDocumentIdentifier textDocument;
    Range                  range;
    FormattingOptions      options;
} DocumentRangeFormattingParams;

OPTIONAL(DocumentRangeFormattingParams);
DA_WITH_NAME(DocumentRangeFormattingParams, DocumentRangeFormattingParamss);
OPTIONAL(DocumentRangeFormattingParamss);

extern OptionalJSONValue                      DocumentRangeFormattingParams_encode(DocumentRangeFormattingParams value);
extern OptionalDocumentRangeFormattingParams  DocumentRangeFormattingParams_decode(OptionalJSONValue json);
extern OptionalJSONValue                      DocumentRangeFormattingParamss_encode(DocumentRangeFormattingParamss value);
extern OptionalDocumentRangeFormattingParamss DocumentRangeFormattingParamss_decode(OptionalJSONValue json);

#endif /* __LSP_DOCUMENTRANGEFORMATTINGPARAMS_H__ */
