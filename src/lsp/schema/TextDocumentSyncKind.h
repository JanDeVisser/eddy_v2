/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_TEXTDOCUMENTSYNCKIND_H__
#define __LSP_TEXTDOCUMENTSYNCKIND_H__

#include <lsp/schema/lsp_base.h>

typedef enum {
    TextDocumentSyncKindNone,
    TextDocumentSyncKindFull,
    TextDocumentSyncKindIncremental,
} TextDocumentSyncKind;

OPTIONAL(TextDocumentSyncKind);
DA_WITH_NAME(TextDocumentSyncKind, TextDocumentSyncKinds);
OPTIONAL(TextDocumentSyncKinds);

extern OptionalJSONValue             TextDocumentSyncKind_encode(TextDocumentSyncKind value);
extern OptionalTextDocumentSyncKind  TextDocumentSyncKind_decode(OptionalJSONValue json);
extern OptionalJSONValue             TextDocumentSyncKinds_encode(TextDocumentSyncKinds value);
extern OptionalTextDocumentSyncKinds TextDocumentSyncKinds_decode(OptionalJSONValue json);
#endif /* __LSP_TEXTDOCUMENTSYNCKIND_H__ */
