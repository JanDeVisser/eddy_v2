/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_TEXTDOCUMENTCLIENTCAPABILITIES_H__
#define __LSP_TEXTDOCUMENTCLIENTCAPABILITIES_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/SemanticTokensClientCapabilities.h>
#include <lsp/schema/TextDocumentSyncClientCapabilities.h>

typedef struct {
    OptionalTextDocumentSyncClientCapabilities synchronization;
    OptionalSemanticTokensClientCapabilities   semanticTokens;
} TextDocumentClientCapabilities;

OPTIONAL(TextDocumentClientCapabilities);
OPTIONAL(OptionalTextDocumentClientCapabilities);
DA_WITH_NAME(TextDocumentClientCapabilities, TextDocumentClientCapabilitiess);
OPTIONAL(TextDocumentClientCapabilitiess);
OPTIONAL(OptionalTextDocumentClientCapabilitiess);

extern OptionalJSONValue                              TextDocumentClientCapabilities_encode(TextDocumentClientCapabilities value);
extern OptionalTextDocumentClientCapabilities         TextDocumentClientCapabilities_decode(OptionalJSONValue json);
extern OptionalJSONValue                              OptionalTextDocumentClientCapabilities_encode(OptionalTextDocumentClientCapabilities value);
extern OptionalOptionalTextDocumentClientCapabilities OptionalTextDocumentClientCapabilities_decode(OptionalJSONValue json);
extern OptionalJSONValue                              TextDocumentClientCapabilitiess_encode(TextDocumentClientCapabilitiess value);
extern OptionalTextDocumentClientCapabilitiess        TextDocumentClientCapabilitiess_decode(OptionalJSONValue json);

#endif /* __LSP_TEXTDOCUMENTCLIENTCAPABILITIES_H__ */
