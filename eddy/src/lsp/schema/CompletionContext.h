/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_COMPLETIONCONTEXT_H__
#define __LSP_COMPLETIONCONTEXT_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/CompletionTriggerKind.h>

typedef struct {
    CompletionTriggerKind triggerKind;
    OptionalStringView    triggerCharacter;
} CompletionContext;

OPTIONAL(CompletionContext);
DA_WITH_NAME(CompletionContext, CompletionContexts);
OPTIONAL(CompletionContexts);

extern OptionalJSONValue          CompletionContext_encode(CompletionContext value);
extern OptionalCompletionContext  CompletionContext_decode(OptionalJSONValue json);
extern OptionalJSONValue          CompletionContexts_encode(CompletionContexts value);
extern OptionalCompletionContexts CompletionContexts_decode(OptionalJSONValue json);

#endif /* __LSP_COMPLETIONCONTEXT_H__ */
