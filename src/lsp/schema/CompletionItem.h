/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_COMPLETIONITEM_H__
#define __LSP_COMPLETIONITEM_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/CompletionItemKind.h>
#include <lsp/schema/CompletionItemLabelDetails.h>
#include <lsp/schema/CompletionItemTag.h>
#include <lsp/schema/InsertReplaceEdit.h>
#include <lsp/schema/InsertTextFormat.h>
#include <lsp/schema/InsertTextMode.h>
#include <lsp/schema/LSPCommand.h>
#include <lsp/schema/MarkupContent.h>
#include <lsp/schema/TextEdit.h>

typedef struct {
    StringView                         label;
    OptionalCompletionItemLabelDetails labelDetails;
    OptionalCompletionItemKind         kind;
    OptionalCompletionItemTags         tags;
    OptionalStringView                 detail;
    struct {
        bool has_value;
        int  tag;
        union {
            StringView    _0;
            MarkupContent _1;
        };
    } documentation;
    OptionalBool             deprecated;
    OptionalBool             preselect;
    OptionalStringView       sortText;
    OptionalStringView       filterText;
    OptionalStringView       insertText;
    OptionalInsertTextFormat insertTextFormat;
    OptionalInsertTextMode   insertTextMode;
    struct {
        bool has_value;
        int  tag;
        union {
            TextEdit          _0;
            InsertReplaceEdit _1;
        };
    } textEdit;
    OptionalStringView  textEditText;
    OptionalTextEdits   additionalTextEdits;
    OptionalStringViews commitCharacters;
    OptionalLSPCommand  command;
    OptionalJSONValue   data;
} CompletionItem;

OPTIONAL(CompletionItem);
DA_WITH_NAME(CompletionItem, CompletionItems);
OPTIONAL(CompletionItems);

extern OptionalJSONValue       CompletionItem_encode(CompletionItem value);
extern OptionalCompletionItem  CompletionItem_decode(OptionalJSONValue json);
extern OptionalJSONValue       CompletionItems_encode(CompletionItems value);
extern OptionalCompletionItems CompletionItems_decode(OptionalJSONValue json);

#endif /* __LSP_COMPLETIONITEM_H__ */
