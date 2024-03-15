/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_SEMANTICTOKENSOPTIONS_H__
#define __LSP_SEMANTICTOKENSOPTIONS_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/SemanticTokensLegend.h>

typedef struct {
    SemanticTokensLegend legend;
    struct {
        bool has_value;
        int  tag;
        union {
            bool _0;
            struct {
                bool has_value;
            } _1;
        };
    } range;
    struct {
        bool has_value;
        int  tag;
        union {
            bool _0;
            struct {
                bool         has_value;
                OptionalBool delta;
            } _1;
        };
    } full;
} SemanticTokensOptions;

OPTIONAL(SemanticTokensOptions);
OPTIONAL(OptionalSemanticTokensOptions);
DA_WITH_NAME(SemanticTokensOptions, SemanticTokensOptionss);
OPTIONAL(SemanticTokensOptionss);
OPTIONAL(OptionalSemanticTokensOptionss);

extern OptionalJSONValue                     SemanticTokensOptions_encode(SemanticTokensOptions value);
extern OptionalSemanticTokensOptions         SemanticTokensOptions_decode(OptionalJSONValue json);
extern OptionalJSONValue                     OptionalSemanticTokensOptions_encode(OptionalSemanticTokensOptions value);
extern OptionalOptionalSemanticTokensOptions OptionalSemanticTokensOptions_decode(OptionalJSONValue json);
extern OptionalJSONValue                     SemanticTokensOptionss_encode(SemanticTokensOptionss value);
extern OptionalSemanticTokensOptionss        SemanticTokensOptionss_decode(OptionalJSONValue json);

#endif /* __LSP_SEMANTICTOKENSOPTIONS_H__ */
