/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_INSERTTEXTMODE_H__
#define __LSP_INSERTTEXTMODE_H__

#include <lsp/schema/lsp_base.h>

typedef enum {
    InsertTextModeAsIs,
    InsertTextModeAdjustIndentation,
} InsertTextMode;

OPTIONAL(InsertTextMode);
DA_WITH_NAME(InsertTextMode, InsertTextModes);
OPTIONAL(InsertTextModes);

extern OptionalJSONValue       InsertTextMode_encode(InsertTextMode value);
extern OptionalInsertTextMode  InsertTextMode_decode(OptionalJSONValue json);
extern OptionalJSONValue       InsertTextModes_encode(InsertTextModes value);
extern OptionalInsertTextModes InsertTextModes_decode(OptionalJSONValue json);
#endif /* __LSP_INSERTTEXTMODE_H__ */
