/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_COMPLETIONLIST_H__
#define __LSP_COMPLETIONLIST_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/CompletionItem.h>
#include <lsp/schema/InsertTextFormat.h>
#include <lsp/schema/InsertTextMode.h>
#include <lsp/schema/Range.h>

typedef struct {
    bool isIncomplete;
    struct {
        bool                has_value;
        OptionalStringViews commitCharacters;
        struct {
            bool has_value;
            int  tag;
            union {
                Range _0;
                struct {
                    bool  has_value;
                    Range insert;
                    Range replace;
                } _1;
            };
        } editRange;
        OptionalInsertTextFormat insertTextFormat;
        OptionalInsertTextMode   insertTextMode;
        OptionalJSONValue        data;
    } itemDefaults;
    CompletionItems items;
} CompletionList;

OPTIONAL(CompletionList);
DA_WITH_NAME(CompletionList, CompletionLists);
OPTIONAL(CompletionLists);

extern OptionalJSONValue       CompletionList_encode(CompletionList value);
extern OptionalCompletionList  CompletionList_decode(OptionalJSONValue json);
extern OptionalJSONValue       CompletionLists_encode(CompletionLists value);
extern OptionalCompletionLists CompletionLists_decode(OptionalJSONValue json);

#endif /* __LSP_COMPLETIONLIST_H__ */
