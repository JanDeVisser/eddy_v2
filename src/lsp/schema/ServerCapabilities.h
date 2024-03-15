/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_SERVERCAPABILITIES_H__
#define __LSP_SERVERCAPABILITIES_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/PositionEncodingKind.h>
#include <lsp/schema/SemanticTokensOptions.h>
#include <lsp/schema/TextDocumentSyncKind.h>
#include <lsp/schema/TextDocumentSyncOptions.h>

typedef struct {
    OptionalPositionEncodingKind positionEncoding;
    struct {
        bool has_value;
        int  tag;
        union {
            TextDocumentSyncOptions _0;
            TextDocumentSyncKind    _1;
        };
    } textDocumentSync;
    OptionalSemanticTokensOptions semanticTokensProvider;
} ServerCapabilities;

OPTIONAL(ServerCapabilities);
OPTIONAL(OptionalServerCapabilities);
DA_WITH_NAME(ServerCapabilities, ServerCapabilitiess);
OPTIONAL(ServerCapabilitiess);
OPTIONAL(OptionalServerCapabilitiess);

extern OptionalJSONValue                  ServerCapabilities_encode(ServerCapabilities value);
extern OptionalServerCapabilities         ServerCapabilities_decode(OptionalJSONValue json);
extern OptionalJSONValue                  OptionalServerCapabilities_encode(OptionalServerCapabilities value);
extern OptionalOptionalServerCapabilities OptionalServerCapabilities_decode(OptionalJSONValue json);
extern OptionalJSONValue                  ServerCapabilitiess_encode(ServerCapabilitiess value);
extern OptionalServerCapabilitiess        ServerCapabilitiess_decode(OptionalJSONValue json);

#endif /* __LSP_SERVERCAPABILITIES_H__ */
