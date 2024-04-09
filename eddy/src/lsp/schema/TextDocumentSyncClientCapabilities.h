/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_TEXTDOCUMENTSYNCCLIENTCAPABILITIES_H__
#define __LSP_TEXTDOCUMENTSYNCCLIENTCAPABILITIES_H__

#include <lsp/schema/lsp_base.h>

typedef struct {
    OptionalBool dynamicRegistration;
    OptionalBool willSave;
    OptionalBool willSaveWaitUntil;
    OptionalBool didSave;
} TextDocumentSyncClientCapabilities;

OPTIONAL(TextDocumentSyncClientCapabilities);
DA_WITH_NAME(TextDocumentSyncClientCapabilities, TextDocumentSyncClientCapabilitiess);
OPTIONAL(TextDocumentSyncClientCapabilitiess);

extern OptionalJSONValue                           TextDocumentSyncClientCapabilities_encode(TextDocumentSyncClientCapabilities value);
extern OptionalTextDocumentSyncClientCapabilities  TextDocumentSyncClientCapabilities_decode(OptionalJSONValue json);
extern OptionalJSONValue                           TextDocumentSyncClientCapabilitiess_encode(TextDocumentSyncClientCapabilitiess value);
extern OptionalTextDocumentSyncClientCapabilitiess TextDocumentSyncClientCapabilitiess_decode(OptionalJSONValue json);

#endif /* __LSP_TEXTDOCUMENTSYNCCLIENTCAPABILITIES_H__ */
