/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_MARKUPKIND_H__
#define __LSP_MARKUPKIND_H__

#include <lsp/schema/lsp_base.h>

typedef enum {
    MarkupKindPlainText,
    MarkupKindMarkdown,
} MarkupKind;

OPTIONAL(MarkupKind);
DA_WITH_NAME(MarkupKind, MarkupKinds);
OPTIONAL(MarkupKinds);

extern OptionalJSONValue   MarkupKind_encode(MarkupKind value);
extern OptionalMarkupKind  MarkupKind_decode(OptionalJSONValue json);
extern OptionalJSONValue   MarkupKinds_encode(MarkupKinds value);
extern OptionalMarkupKinds MarkupKinds_decode(OptionalJSONValue json);
extern StringView          MarkupKind_to_string(MarkupKind value);
extern OptionalMarkupKind  MarkupKind_parse(StringView s);
#endif /* __LSP_MARKUPKIND_H__ */
