/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_TEXTDOCUMENTIDENTIFIER_H__
#define __LSP_TEXTDOCUMENTIDENTIFIER_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/DocumentUri.h>

typedef struct {
    DocumentUri uri;
} TextDocumentIdentifier;

OPTIONAL(TextDocumentIdentifier);
OPTIONAL(OptionalTextDocumentIdentifier);
DA_WITH_NAME(TextDocumentIdentifier, TextDocumentIdentifiers);
OPTIONAL(TextDocumentIdentifiers);
OPTIONAL(OptionalTextDocumentIdentifiers);

extern OptionalJSONValue                      TextDocumentIdentifier_encode(TextDocumentIdentifier value);
extern OptionalTextDocumentIdentifier         TextDocumentIdentifier_decode(OptionalJSONValue json);
extern OptionalJSONValue                      OptionalTextDocumentIdentifier_encode(OptionalTextDocumentIdentifier value);
extern OptionalOptionalTextDocumentIdentifier OptionalTextDocumentIdentifier_decode(OptionalJSONValue json);
extern OptionalJSONValue                      TextDocumentIdentifiers_encode(TextDocumentIdentifiers value);
extern OptionalTextDocumentIdentifiers        TextDocumentIdentifiers_decode(OptionalJSONValue json);

#endif /* __LSP_TEXTDOCUMENTIDENTIFIER_H__ */
