/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_TEXTEDIT_H__
#define __LSP_TEXTEDIT_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/Range.h>

typedef struct {
    Range      range;
    StringView newText;
} TextEdit;

OPTIONAL(TextEdit);
OPTIONAL(OptionalTextEdit);
DA_WITH_NAME(TextEdit, TextEdits);
OPTIONAL(TextEdits);
OPTIONAL(OptionalTextEdits);

extern OptionalJSONValue        TextEdit_encode(TextEdit value);
extern OptionalTextEdit         TextEdit_decode(OptionalJSONValue json);
extern OptionalJSONValue        OptionalTextEdit_encode(OptionalTextEdit value);
extern OptionalOptionalTextEdit OptionalTextEdit_decode(OptionalJSONValue json);
extern OptionalJSONValue        TextEdits_encode(TextEdits value);
extern OptionalTextEdits        TextEdits_decode(OptionalJSONValue json);

#endif /* __LSP_TEXTEDIT_H__ */
