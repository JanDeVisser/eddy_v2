/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_COMPLETIONTRIGGERKIND_H__
#define __LSP_COMPLETIONTRIGGERKIND_H__

#include <lsp/schema/lsp_base.h>

typedef enum {
    CompletionTriggerKindInvoked,
    CompletionTriggerKindTriggerCharacter,
    CompletionTriggerKindTriggerForIncompleteCompletions,
} CompletionTriggerKind;

OPTIONAL(CompletionTriggerKind);
DA_WITH_NAME(CompletionTriggerKind, CompletionTriggerKinds);
OPTIONAL(CompletionTriggerKinds);

extern OptionalJSONValue              CompletionTriggerKind_encode(CompletionTriggerKind value);
extern OptionalCompletionTriggerKind  CompletionTriggerKind_decode(OptionalJSONValue json);
extern OptionalJSONValue              CompletionTriggerKinds_encode(CompletionTriggerKinds value);
extern OptionalCompletionTriggerKinds CompletionTriggerKinds_decode(OptionalJSONValue json);
#endif /* __LSP_COMPLETIONTRIGGERKIND_H__ */
