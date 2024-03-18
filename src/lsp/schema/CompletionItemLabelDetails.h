/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_COMPLETIONITEMLABELDETAILS_H__
#define __LSP_COMPLETIONITEMLABELDETAILS_H__

#include <lsp/schema/lsp_base.h>

typedef struct {
    OptionalStringView detail;
    OptionalStringView description;
} CompletionItemLabelDetails;

OPTIONAL(CompletionItemLabelDetails);
DA_WITH_NAME(CompletionItemLabelDetails, CompletionItemLabelDetailss);
OPTIONAL(CompletionItemLabelDetailss);

extern OptionalJSONValue                   CompletionItemLabelDetails_encode(CompletionItemLabelDetails value);
extern OptionalCompletionItemLabelDetails  CompletionItemLabelDetails_decode(OptionalJSONValue json);
extern OptionalJSONValue                   CompletionItemLabelDetailss_encode(CompletionItemLabelDetailss value);
extern OptionalCompletionItemLabelDetailss CompletionItemLabelDetailss_decode(OptionalJSONValue json);

#endif /* __LSP_COMPLETIONITEMLABELDETAILS_H__ */
