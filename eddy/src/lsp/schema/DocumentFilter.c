/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/DocumentFilter.h>

DA_IMPL(DocumentFilter)

OptionalJSONValue DocumentFilters_encode(DocumentFilters value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, DocumentFilter_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalDocumentFilters DocumentFilters_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(DocumentFilters);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    DocumentFilters ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue      elem = json_at(&json.value, ix);
        OptionalDocumentFilter val = DocumentFilter_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(DocumentFilters);
        }
        da_append_DocumentFilter(&ret, val.value);
    }
    RETURN_VALUE(DocumentFilters, ret);
}

OptionalDocumentFilter DocumentFilter_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(DocumentFilter);
    }
    DocumentFilter value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "language");
        value.language = StringView_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "scheme");
        value.scheme = StringView_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "pattern");
        value.pattern = StringView_decode(v0);
    }
    RETURN_VALUE(DocumentFilter, value);
}

OptionalJSONValue DocumentFilter_encode(DocumentFilter value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.language.has_value) {
            _encoded_maybe = StringView_encode(value.language.value);
        }
        json_optional_set(&v1, "language", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.scheme.has_value) {
            _encoded_maybe = StringView_encode(value.scheme.value);
        }
        json_optional_set(&v1, "scheme", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.pattern.has_value) {
            _encoded_maybe = StringView_encode(value.pattern.value);
        }
        json_optional_set(&v1, "pattern", _encoded_maybe);
    }
    RETURN_VALUE(JSONValue, v1);
}
