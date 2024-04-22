/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_MARKUPCONTENT_H__
#define __LSP_MARKUPCONTENT_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/MarkupKind.h>

typedef struct {
    MarkupKind kind;
    StringView value;
} MarkupContent;

OPTIONAL(MarkupContent);
DA_WITH_NAME(MarkupContent, MarkupContents);
OPTIONAL(MarkupContents);

extern OptionalJSONValue      MarkupContent_encode(MarkupContent value);
extern OptionalMarkupContent  MarkupContent_decode(OptionalJSONValue json);
extern OptionalJSONValue      MarkupContents_encode(MarkupContents value);
extern OptionalMarkupContents MarkupContents_decode(OptionalJSONValue json);

#endif /* __LSP_MARKUPCONTENT_H__ */
