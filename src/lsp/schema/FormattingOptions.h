/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_FORMATTINGOPTIONS_H__
#define __LSP_FORMATTINGOPTIONS_H__

#include <lsp/schema/lsp_base.h>

typedef struct {
    unsigned int tabSize;
    bool         insertSpaces;
    OptionalBool trimTrailingWhitespace;
    OptionalBool insertFinalNewline;
    OptionalBool trimFinalNewlines;
} FormattingOptions;

OPTIONAL(FormattingOptions);
OPTIONAL(OptionalFormattingOptions);
DA_WITH_NAME(FormattingOptions, FormattingOptionss);
OPTIONAL(FormattingOptionss);
OPTIONAL(OptionalFormattingOptionss);

extern OptionalJSONValue                 FormattingOptions_encode(FormattingOptions value);
extern OptionalFormattingOptions         FormattingOptions_decode(OptionalJSONValue json);
extern OptionalJSONValue                 OptionalFormattingOptions_encode(OptionalFormattingOptions value);
extern OptionalOptionalFormattingOptions OptionalFormattingOptions_decode(OptionalJSONValue json);
extern OptionalJSONValue                 FormattingOptionss_encode(FormattingOptionss value);
extern OptionalFormattingOptionss        FormattingOptionss_decode(OptionalJSONValue json);

#endif /* __LSP_FORMATTINGOPTIONS_H__ */
