/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_SEMANTICTOKENSCLIENTCAPABILITIES_H__
#define __LSP_SEMANTICTOKENSCLIENTCAPABILITIES_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/TokenFormat.h>

typedef struct {
    OptionalBool dynamicRegistration;
    struct {
        bool has_value;
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
    } requests;
    StringViews  tokenTypes;
    StringViews  tokenModifiers;
    TokenFormats formats;
    OptionalBool overlappingTokenSupport;
    OptionalBool multilineTokenSupport;
    OptionalBool serverCancelSupport;
    OptionalBool augmentsSyntaxTokens;
} SemanticTokensClientCapabilities;

OPTIONAL(SemanticTokensClientCapabilities);
DA_WITH_NAME(SemanticTokensClientCapabilities, SemanticTokensClientCapabilitiess);
OPTIONAL(SemanticTokensClientCapabilitiess);

extern OptionalJSONValue                         SemanticTokensClientCapabilities_encode(SemanticTokensClientCapabilities value);
extern OptionalSemanticTokensClientCapabilities  SemanticTokensClientCapabilities_decode(OptionalJSONValue json);
extern OptionalJSONValue                         SemanticTokensClientCapabilitiess_encode(SemanticTokensClientCapabilitiess value);
extern OptionalSemanticTokensClientCapabilitiess SemanticTokensClientCapabilitiess_decode(OptionalJSONValue json);

#endif /* __LSP_SEMANTICTOKENSCLIENTCAPABILITIES_H__ */
