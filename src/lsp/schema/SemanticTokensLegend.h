/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_SEMANTICTOKENSLEGEND_H__
#define __LSP_SEMANTICTOKENSLEGEND_H__

#include <lsp/schema/lsp_base.h>

typedef struct {
    StringViews tokenTypes;
    StringViews tokenModifiers;
} SemanticTokensLegend;

OPTIONAL(SemanticTokensLegend);
OPTIONAL(OptionalSemanticTokensLegend);
DA_WITH_NAME(SemanticTokensLegend, SemanticTokensLegends);
OPTIONAL(SemanticTokensLegends);
OPTIONAL(OptionalSemanticTokensLegends);

extern OptionalJSONValue                    SemanticTokensLegend_encode(SemanticTokensLegend value);
extern OptionalSemanticTokensLegend         SemanticTokensLegend_decode(OptionalJSONValue json);
extern OptionalJSONValue                    OptionalSemanticTokensLegend_encode(OptionalSemanticTokensLegend value);
extern OptionalOptionalSemanticTokensLegend OptionalSemanticTokensLegend_decode(OptionalJSONValue json);
extern OptionalJSONValue                    SemanticTokensLegends_encode(SemanticTokensLegends value);
extern OptionalSemanticTokensLegends        SemanticTokensLegends_decode(OptionalJSONValue json);

#endif /* __LSP_SEMANTICTOKENSLEGEND_H__ */
