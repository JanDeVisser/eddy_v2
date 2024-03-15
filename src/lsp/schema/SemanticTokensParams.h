/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_SEMANTICTOKENSPARAMS_H__
#define __LSP_SEMANTICTOKENSPARAMS_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/TextDocumentIdentifier.h>

typedef struct {
    TextDocumentIdentifier textDocument;
} SemanticTokensParams;

OPTIONAL(SemanticTokensParams);
OPTIONAL(OptionalSemanticTokensParams);
DA_WITH_NAME(SemanticTokensParams, SemanticTokensParamss);
OPTIONAL(SemanticTokensParamss);
OPTIONAL(OptionalSemanticTokensParamss);

extern OptionalJSONValue                    SemanticTokensParams_encode(SemanticTokensParams value);
extern OptionalSemanticTokensParams         SemanticTokensParams_decode(OptionalJSONValue json);
extern OptionalJSONValue                    OptionalSemanticTokensParams_encode(OptionalSemanticTokensParams value);
extern OptionalOptionalSemanticTokensParams OptionalSemanticTokensParams_decode(OptionalJSONValue json);
extern OptionalJSONValue                    SemanticTokensParamss_encode(SemanticTokensParamss value);
extern OptionalSemanticTokensParamss        SemanticTokensParamss_decode(OptionalJSONValue json);

#endif /* __LSP_SEMANTICTOKENSPARAMS_H__ */
