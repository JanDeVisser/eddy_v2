/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/DocumentFilter.h>

DA_IMPL(DocumentFilter)

OptionalJSONValue OptionalDocumentFilter_encode(OptionalDocumentFilter value)
{
    if (value.has_value) {
        return DocumentFilter_encode(value.value);
    } else {
        RETURN_EMPTY(JSONValue);
    }
}

OptionalOptionalDocumentFilter OptionalDocumentFilter_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(OptionalDocumentFilter);
    }
    RETURN_VALUE(OptionalDocumentFilter, DocumentFilter_decode(json));
}

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
        value.language = FORWARD_OPTIONAL(OptionalStringView, DocumentFilter, OptionalStringView_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "scheme");
        value.scheme = FORWARD_OPTIONAL(OptionalStringView, DocumentFilter, OptionalStringView_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "pattern");
        value.pattern = FORWARD_OPTIONAL(OptionalStringView, DocumentFilter, OptionalStringView_decode(v0));
    }
    RETURN_VALUE(DocumentFilter, value);
}
OptionalJSONValue DocumentFilter_encode(DocumentFilter value)
{
    JSONValue v1 = json_object();
    json_optional_set(&v1, "language", OptionalStringView_encode(value.language));
    json_optional_set(&v1, "scheme", OptionalStringView_encode(value.scheme));
    json_optional_set(&v1, "pattern", OptionalStringView_encode(value.pattern));
    RETURN_VALUE(JSONValue, v1);
}
