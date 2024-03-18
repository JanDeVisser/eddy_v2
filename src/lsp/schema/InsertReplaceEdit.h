/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_INSERTREPLACEEDIT_H__
#define __LSP_INSERTREPLACEEDIT_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/Range.h>

typedef struct {
    StringView newText;
    Range      insert;
    Range      replace;
} InsertReplaceEdit;

OPTIONAL(InsertReplaceEdit);
DA_WITH_NAME(InsertReplaceEdit, InsertReplaceEdits);
OPTIONAL(InsertReplaceEdits);

extern OptionalJSONValue          InsertReplaceEdit_encode(InsertReplaceEdit value);
extern OptionalInsertReplaceEdit  InsertReplaceEdit_decode(OptionalJSONValue json);
extern OptionalJSONValue          InsertReplaceEdits_encode(InsertReplaceEdits value);
extern OptionalInsertReplaceEdits InsertReplaceEdits_decode(OptionalJSONValue json);

#endif /* __LSP_INSERTREPLACEEDIT_H__ */
