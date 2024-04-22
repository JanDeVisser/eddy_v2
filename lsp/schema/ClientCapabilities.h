/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_CLIENTCAPABILITIES_H__
#define __LSP_CLIENTCAPABILITIES_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/TextDocumentClientCapabilities.h>

typedef struct {
    OptionalTextDocumentClientCapabilities textDocument;
} ClientCapabilities;

OPTIONAL(ClientCapabilities);
DA_WITH_NAME(ClientCapabilities, ClientCapabilitiess);
OPTIONAL(ClientCapabilitiess);

extern OptionalJSONValue           ClientCapabilities_encode(ClientCapabilities value);
extern OptionalClientCapabilities  ClientCapabilities_decode(OptionalJSONValue json);
extern OptionalJSONValue           ClientCapabilitiess_encode(ClientCapabilitiess value);
extern OptionalClientCapabilitiess ClientCapabilitiess_decode(OptionalJSONValue json);

#endif /* __LSP_CLIENTCAPABILITIES_H__ */
