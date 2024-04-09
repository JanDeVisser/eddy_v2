/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_COMPLETIONITEMKIND_H__
#define __LSP_COMPLETIONITEMKIND_H__

#include <lsp/schema/lsp_base.h>

typedef enum {
    CompletionItemKindText,
    CompletionItemKindMethod,
    CompletionItemKindFunction,
    CompletionItemKindConstructor,
    CompletionItemKindField,
    CompletionItemKindVariable,
    CompletionItemKindClass,
    CompletionItemKindInterface,
    CompletionItemKindModule,
    CompletionItemKindProperty,
    CompletionItemKindUnit,
    CompletionItemKindValue,
    CompletionItemKindEnum,
    CompletionItemKindKeyword,
    CompletionItemKindSnippet,
    CompletionItemKindColor,
    CompletionItemKindFile,
    CompletionItemKindReference,
    CompletionItemKindFolder,
    CompletionItemKindEnumMember,
    CompletionItemKindConstant,
    CompletionItemKindStruct,
    CompletionItemKindEvent,
    CompletionItemKindOperator,
    CompletionItemKindTypeParameter,
} CompletionItemKind;

OPTIONAL(CompletionItemKind);
DA_WITH_NAME(CompletionItemKind, CompletionItemKinds);
OPTIONAL(CompletionItemKinds);

extern OptionalJSONValue           CompletionItemKind_encode(CompletionItemKind value);
extern OptionalCompletionItemKind  CompletionItemKind_decode(OptionalJSONValue json);
extern OptionalJSONValue           CompletionItemKinds_encode(CompletionItemKinds value);
extern OptionalCompletionItemKinds CompletionItemKinds_decode(OptionalJSONValue json);
#endif /* __LSP_COMPLETIONITEMKIND_H__ */
