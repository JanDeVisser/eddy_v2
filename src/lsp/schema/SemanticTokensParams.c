/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/SemanticTokensParams.h>

DA_IMPL(SemanticTokensParams)

OptionalJSONValue OptionalSemanticTokensParams_encode(OptionalSemanticTokensParams value)
{
    if (value.has_value) {
        return SemanticTokensParams_encode(value.value);
    } else {
        RETURN_EMPTY(JSONValue);
    }
}

OptionalOptionalSemanticTokensParams OptionalSemanticTokensParams_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(OptionalSemanticTokensParams);
    }
    RETURN_VALUE(OptionalSemanticTokensParams, SemanticTokensParams_decode(json));
}

OptionalJSONValue SemanticTokensParamss_encode(SemanticTokensParamss value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, SemanticTokensParams_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalSemanticTokensParamss SemanticTokensParamss_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(SemanticTokensParamss);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    SemanticTokensParamss ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue            elem = json_at(&json.value, ix);
        OptionalSemanticTokensParams val = SemanticTokensParams_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(SemanticTokensParamss);
        }
        da_append_SemanticTokensParams(&ret, val.value);
    }
    RETURN_VALUE(SemanticTokensParamss, ret);
}
OptionalSemanticTokensParams SemanticTokensParams_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(SemanticTokensParams);
    }
    SemanticTokensParams value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "textDocument");
        value.textDocument = FORWARD_OPTIONAL(TextDocumentIdentifier, SemanticTokensParams, TextDocumentIdentifier_decode(v0));
    }
    RETURN_VALUE(SemanticTokensParams, value);
}
OptionalJSONValue SemanticTokensParams_encode(SemanticTokensParams value)
{
    JSONValue v1 = json_object();
    json_optional_set(&v1, "textDocument", TextDocumentIdentifier_encode(value.textDocument));
    RETURN_VALUE(JSONValue, v1);
}
