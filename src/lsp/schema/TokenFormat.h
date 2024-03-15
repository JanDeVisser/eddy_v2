/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_TOKENFORMAT_H__
#define __LSP_TOKENFORMAT_H__

#include <lsp/schema/lsp_base.h>

typedef enum {
    TokenFormatRelative,
} TokenFormat;

OPTIONAL(TokenFormat);
OPTIONAL(OptionalTokenFormat);
DA_WITH_NAME(TokenFormat, TokenFormats);
OPTIONAL(TokenFormats);
OPTIONAL(OptionalTokenFormats);

extern OptionalJSONValue            TokenFormat_encode(TokenFormat value);
extern OptionalTokenFormat          TokenFormat_decode(OptionalJSONValue json);
extern OptionalJSONValue            OptionalTokenFormat_encode(OptionalTokenFormat value);
extern OptionalOptionalTokenFormat  OptionalTokenFormat_decode(OptionalJSONValue json);
extern OptionalJSONValue            TokenFormats_encode(TokenFormats value);
extern OptionalTokenFormats         TokenFormats_decode(OptionalJSONValue json);
extern OptionalJSONValue            OptionalTokenFormats_encode(OptionalTokenFormats value);
extern OptionalOptionalTokenFormats OptionalTokenFormats_decode(OptionalJSONValue json);
extern StringView                   TokenFormat_to_string(TokenFormat value);
extern OptionalTokenFormat          TokenFormat_parse(StringView s);
#endif /* __LSP_TOKENFORMAT_H__ */
