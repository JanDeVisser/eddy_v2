/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_TEXTDOCUMENTSYNCOPTIONS_H__
#define __LSP_TEXTDOCUMENTSYNCOPTIONS_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/SaveOptions.h>
#include <lsp/schema/TextDocumentSyncKind.h>

typedef struct {
    OptionalBool                 openClose;
    OptionalTextDocumentSyncKind change;
    OptionalBool                 willSave;
    OptionalBool                 willSaveWaitUntil;
    struct {
        bool has_value;
        int  tag;
        union {
            bool        _0;
            SaveOptions _1;
        };
    } save;
} TextDocumentSyncOptions;

OPTIONAL(TextDocumentSyncOptions);
DA_WITH_NAME(TextDocumentSyncOptions, TextDocumentSyncOptionss);
OPTIONAL(TextDocumentSyncOptionss);

extern OptionalJSONValue                TextDocumentSyncOptions_encode(TextDocumentSyncOptions value);
extern OptionalTextDocumentSyncOptions  TextDocumentSyncOptions_decode(OptionalJSONValue json);
extern OptionalJSONValue                TextDocumentSyncOptionss_encode(TextDocumentSyncOptionss value);
extern OptionalTextDocumentSyncOptionss TextDocumentSyncOptionss_decode(OptionalJSONValue json);

#endif /* __LSP_TEXTDOCUMENTSYNCOPTIONS_H__ */
