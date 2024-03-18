/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_COMPLETIONPARAMS_H__
#define __LSP_COMPLETIONPARAMS_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/CompletionContext.h>
#include <lsp/schema/TextDocumentPositionParams.h>
#include <lsp/schema/WorkDoneProgressParams.h>

typedef struct {
    TextDocumentIdentifier    textDocument;
    Position                  position;
    OptionalCompletionContext context;
} CompletionParams;

OPTIONAL(CompletionParams);
DA_WITH_NAME(CompletionParams, CompletionParamss);
OPTIONAL(CompletionParamss);

extern OptionalJSONValue         CompletionParams_encode(CompletionParams value);
extern OptionalCompletionParams  CompletionParams_decode(OptionalJSONValue json);
extern OptionalJSONValue         CompletionParamss_encode(CompletionParamss value);
extern OptionalCompletionParamss CompletionParamss_decode(OptionalJSONValue json);

#endif /* __LSP_COMPLETIONPARAMS_H__ */
