#include "json.h"
/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/DocumentRangeFormattingParams.h>

DA_IMPL(DocumentRangeFormattingParams)

OptionalJSONValue DocumentRangeFormattingParamss_encode(DocumentRangeFormattingParamss value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, DocumentRangeFormattingParams_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalDocumentRangeFormattingParamss DocumentRangeFormattingParamss_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(DocumentRangeFormattingParamss);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    DocumentRangeFormattingParamss ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue                     elem = json_at(&json.value, ix);
        OptionalDocumentRangeFormattingParams val = DocumentRangeFormattingParams_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(DocumentRangeFormattingParamss);
        }
        da_append_DocumentRangeFormattingParams(&ret, val.value);
    }
    RETURN_VALUE(DocumentRangeFormattingParamss, ret);
}

OptionalDocumentRangeFormattingParams DocumentRangeFormattingParams_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(DocumentRangeFormattingParams);
    }
    DocumentRangeFormattingParams value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "textDocument");
        value.textDocument = FORWARD_OPTIONAL(TextDocumentIdentifier, DocumentRangeFormattingParams, TextDocumentIdentifier_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "range");
        value.range = FORWARD_OPTIONAL(Range, DocumentRangeFormattingParams, Range_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "options");
        value.options = FORWARD_OPTIONAL(FormattingOptions, DocumentRangeFormattingParams, FormattingOptions_decode(v0));
    }
    RETURN_VALUE(DocumentRangeFormattingParams, value);
}

OptionalJSONValue DocumentRangeFormattingParams_encode(DocumentRangeFormattingParams value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = TextDocumentIdentifier_encode(value.textDocument);
        json_optional_set(&v1, "textDocument", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = Range_encode(value.range);
        json_optional_set(&v1, "range", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = FormattingOptions_encode(value.options);
        json_optional_set(&v1, "options", _encoded_maybe);
    }
    RETURN_VALUE(JSONValue, v1);
}
