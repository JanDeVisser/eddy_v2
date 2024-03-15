/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/DocumentFormattingParams.h>

DA_IMPL(DocumentFormattingParams)

OptionalJSONValue OptionalDocumentFormattingParams_encode(OptionalDocumentFormattingParams value)
{
    if (value.has_value) {
        return DocumentFormattingParams_encode(value.value);
    } else {
        RETURN_EMPTY(JSONValue);
    }
}

OptionalOptionalDocumentFormattingParams OptionalDocumentFormattingParams_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(OptionalDocumentFormattingParams);
    }
    RETURN_VALUE(OptionalDocumentFormattingParams, DocumentFormattingParams_decode(json));
}

OptionalJSONValue DocumentFormattingParamss_encode(DocumentFormattingParamss value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, DocumentFormattingParams_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalDocumentFormattingParamss DocumentFormattingParamss_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(DocumentFormattingParamss);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    DocumentFormattingParamss ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue                elem = json_at(&json.value, ix);
        OptionalDocumentFormattingParams val = DocumentFormattingParams_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(DocumentFormattingParamss);
        }
        da_append_DocumentFormattingParams(&ret, val.value);
    }
    RETURN_VALUE(DocumentFormattingParamss, ret);
}
OptionalDocumentFormattingParams DocumentFormattingParams_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(DocumentFormattingParams);
    }
    DocumentFormattingParams value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "textDocument");
        value.textDocument = FORWARD_OPTIONAL(TextDocumentIdentifier, DocumentFormattingParams, TextDocumentIdentifier_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "options");
        value.options = FORWARD_OPTIONAL(FormattingOptions, DocumentFormattingParams, FormattingOptions_decode(v0));
    }
    RETURN_VALUE(DocumentFormattingParams, value);
}
OptionalJSONValue DocumentFormattingParams_encode(DocumentFormattingParams value)
{
    JSONValue v1 = json_object();
    json_optional_set(&v1, "textDocument", TextDocumentIdentifier_encode(value.textDocument));
    json_optional_set(&v1, "options", FormattingOptions_encode(value.options));
    RETURN_VALUE(JSONValue, v1);
}
