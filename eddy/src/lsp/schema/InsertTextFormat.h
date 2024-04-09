/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_INSERTTEXTFORMAT_H__
#define __LSP_INSERTTEXTFORMAT_H__

#include <lsp/schema/lsp_base.h>

typedef enum {
    InsertTextFormatPlainText,
    InsertTextFormatSnippet,
} InsertTextFormat;

OPTIONAL(InsertTextFormat);
DA_WITH_NAME(InsertTextFormat, InsertTextFormats);
OPTIONAL(InsertTextFormats);

extern OptionalJSONValue         InsertTextFormat_encode(InsertTextFormat value);
extern OptionalInsertTextFormat  InsertTextFormat_decode(OptionalJSONValue json);
extern OptionalJSONValue         InsertTextFormats_encode(InsertTextFormats value);
extern OptionalInsertTextFormats InsertTextFormats_decode(OptionalJSONValue json);
#endif /* __LSP_INSERTTEXTFORMAT_H__ */
