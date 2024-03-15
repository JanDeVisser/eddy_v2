/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_TEXTDOCUMENTITEM_H__
#define __LSP_TEXTDOCUMENTITEM_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/DocumentUri.h>

typedef struct {
    DocumentUri uri;
    StringView  languageId;
    int         version;
    StringView  text;
} TextDocumentItem;

OPTIONAL(TextDocumentItem);
OPTIONAL(OptionalTextDocumentItem);
DA_WITH_NAME(TextDocumentItem, TextDocumentItems);
OPTIONAL(TextDocumentItems);
OPTIONAL(OptionalTextDocumentItems);

extern OptionalJSONValue                TextDocumentItem_encode(TextDocumentItem value);
extern OptionalTextDocumentItem         TextDocumentItem_decode(OptionalJSONValue json);
extern OptionalJSONValue                OptionalTextDocumentItem_encode(OptionalTextDocumentItem value);
extern OptionalOptionalTextDocumentItem OptionalTextDocumentItem_decode(OptionalJSONValue json);
extern OptionalJSONValue                TextDocumentItems_encode(TextDocumentItems value);
extern OptionalTextDocumentItems        TextDocumentItems_decode(OptionalJSONValue json);

#endif /* __LSP_TEXTDOCUMENTITEM_H__ */
