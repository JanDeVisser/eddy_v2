/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_SEMANTICTOKENS_H__
#define __LSP_SEMANTICTOKENS_H__

#include <lsp/schema/lsp_base.h>

typedef struct {
    OptionalStringView resultId;
    UInt32s            data;
} SemanticTokens;

OPTIONAL(SemanticTokens);
DA_WITH_NAME(SemanticTokens, SemanticTokenss);
OPTIONAL(SemanticTokenss);

extern OptionalJSONValue       SemanticTokens_encode(SemanticTokens value);
extern OptionalSemanticTokens  SemanticTokens_decode(OptionalJSONValue json);
extern OptionalJSONValue       SemanticTokenss_encode(SemanticTokenss value);
extern OptionalSemanticTokenss SemanticTokenss_decode(OptionalJSONValue json);

#endif /* __LSP_SEMANTICTOKENS_H__ */
