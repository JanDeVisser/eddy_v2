/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */
 
// clang-format off 

#ifndef __LSP_LANGUAGE_H__
#define __LSP_LANGUAGE_H__

#include <lsp/lsp_base.h>
#include <lsp/base.h>

typedef struct {
    int tabSize;
    bool insertSpaces;
    OptionalBool trimTrailingWhitespace;
    OptionalBool insertFinalNewline;
    OptionalBool trimFinalNewlines;
} FormattingOptions;

extern OptionalJSONValue FormattingOptions_encode(FormattingOptions value);

typedef struct {
    TextDocumentIdentifier textDocument;
    FormattingOptions options;
} DocumentFormattingParams;

extern OptionalJSONValue DocumentFormattingParams_encode(DocumentFormattingParams value);


// clang-format on
#endif /* __LSP_LANGUAGE_H__ */

