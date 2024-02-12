/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */
 
// clang-format off 

#ifndef __LSP_SEMANTICTOKENS_H__
#define __LSP_SEMANTICTOKENS_H__

#include <lsp/lsp_base.h>
#include <lsp/base.h>

typedef struct {
    OptionalStringView resultId;
    UInts data;
} SemanticTokens;

extern SemanticTokens SemanticTokens_decode(OptionalJSONValue value);
extern OptionalJSONValue SemanticTokens_encode(SemanticTokens value);

typedef struct {
    int tag;
    union {
        SemanticTokens _0;
        Null _1;
    };
} SemanticTokensResult;

extern SemanticTokensResult SemanticTokensResult_decode(OptionalJSONValue value);
extern OptionalJSONValue SemanticTokensResult_encode(SemanticTokensResult value);

typedef struct {
    TextDocumentIdentifier textDocument;
} SemanticTokensParams;

extern OptionalJSONValue SemanticTokensParams_encode(SemanticTokensParams value);


// clang-format on
#endif /* __LSP_SEMANTICTOKENS_H__ */

