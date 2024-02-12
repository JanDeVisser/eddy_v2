/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */
 
// clang-format off 

#include <lsp/semantictokens.h>

SemanticTokens SemanticTokens_decode(OptionalJSONValue v4)
{
    assert(v4.has_value);
    assert(v4.value.type == JSON_TYPE_OBJECT);
    SemanticTokens value = {0};
    {
        OptionalJSONValue v8 = json_get(&v4.value, "resultId");
        value.resultId = OptionalStringView_decode(v8);
    }
    {
        OptionalJSONValue v8 = json_get(&v4.value, "data");
        value.data = UInts_decode(v8);
    }
    return value;
}

OptionalJSONValue SemanticTokens_encode(SemanticTokens value)
{
    JSONValue v4 = json_object();
    json_optional_set(&v4, "resultId", OptionalStringView_encode(value.resultId));
    json_optional_set(&v4, "data", UInts_encode(value.data));
    RETURN_VALUE(JSONValue, v4);
}

SemanticTokensResult SemanticTokensResult_decode(OptionalJSONValue v4)
{
    assert(v4.has_value);
    SemanticTokensResult value = {0};
    assert(v4.has_value);
    if (v4.value.type == JSON_TYPE_OBJECT) {
        value.tag = 0;
        value._0 = SemanticTokens_decode(v4);
    }
    if (v4.value.type == JSON_TYPE_NULL) {
        value.tag = 1;
        value._1 = (Null) {};
    }
    return value;
}

OptionalJSONValue SemanticTokensParams_encode(SemanticTokensParams value)
{
    JSONValue v4 = json_object();
    json_optional_set(&v4, "textDocument", TextDocumentIdentifier_encode(value.textDocument));
    RETURN_VALUE(JSONValue, v4);
}

// clang-format on

