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
OPTIONAL(OptionalTextDocumentSyncKind);
DA_WITH_NAME(TextDocumentSyncKind, TextDocumentSyncKinds);
OPTIONAL(TextDocumentSyncKinds);
OPTIONAL(OptionalTextDocumentSyncKinds);

extern OptionalJSONValue                     TextDocumentSyncKind_encode(TextDocumentSyncKind value);
extern OptionalTextDocumentSyncKind          TextDocumentSyncKind_decode(OptionalJSONValue json);
extern OptionalJSONValue                     OptionalTextDocumentSyncKind_encode(OptionalTextDocumentSyncKind value);
extern OptionalOptionalTextDocumentSyncKind  OptionalTextDocumentSyncKind_decode(OptionalJSONValue json);
extern OptionalJSONValue                     TextDocumentSyncKinds_encode(TextDocumentSyncKinds value);
extern OptionalTextDocumentSyncKinds         TextDocumentSyncKinds_decode(OptionalJSONValue json);
extern OptionalJSONValue                     OptionalTextDocumentSyncKinds_encode(OptionalTextDocumentSyncKinds value);
extern OptionalOptionalTextDocumentSyncKinds OptionalTextDocumentSyncKinds_decode(OptionalJSONValue json);
#endif /* __LSP_TEXTDOCUMENTSYNCKIND_H__ */
