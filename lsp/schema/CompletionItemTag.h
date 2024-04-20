/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_COMPLETIONITEMTAG_H__
#define __LSP_COMPLETIONITEMTAG_H__

#include <lsp/schema/lsp_base.h>

typedef enum {
    CompletionItemTagDeprecated,
} CompletionItemTag;

OPTIONAL(CompletionItemTag);
DA_WITH_NAME(CompletionItemTag, CompletionItemTags);
OPTIONAL(CompletionItemTags);

extern OptionalJSONValue          CompletionItemTag_encode(CompletionItemTag value);
extern OptionalCompletionItemTag  CompletionItemTag_decode(OptionalJSONValue json);
extern OptionalJSONValue          CompletionItemTags_encode(CompletionItemTags value);
extern OptionalCompletionItemTags CompletionItemTags_decode(OptionalJSONValue json);
#endif /* __LSP_COMPLETIONITEMTAG_H__ */
