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
DA_WITH_NAME(TokenFormat, TokenFormats);
OPTIONAL(TokenFormats);

extern OptionalJSONValue    TokenFormat_encode(TokenFormat value);
extern OptionalTokenFormat  TokenFormat_decode(OptionalJSONValue json);
extern OptionalJSONValue    TokenFormats_encode(TokenFormats value);
extern OptionalTokenFormats TokenFormats_decode(OptionalJSONValue json);
extern StringView           TokenFormat_to_string(TokenFormat value);
extern OptionalTokenFormat  TokenFormat_parse(StringView s);
#endif /* __LSP_TOKENFORMAT_H__ */
