#include "json.h"
/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/TextDocumentSyncClientCapabilities.h>

DA_IMPL(TextDocumentSyncClientCapabilities)

OptionalJSONValue TextDocumentSyncClientCapabilitiess_encode(TextDocumentSyncClientCapabilitiess value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, TextDocumentSyncClientCapabilities_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalTextDocumentSyncClientCapabilitiess TextDocumentSyncClientCapabilitiess_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(TextDocumentSyncClientCapabilitiess);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    TextDocumentSyncClientCapabilitiess ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue                          elem = json_at(&json.value, ix);
        OptionalTextDocumentSyncClientCapabilities val = TextDocumentSyncClientCapabilities_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(TextDocumentSyncClientCapabilitiess);
        }
        da_append_TextDocumentSyncClientCapabilities(&ret, val.value);
    }
    RETURN_VALUE(TextDocumentSyncClientCapabilitiess, ret);
}

OptionalTextDocumentSyncClientCapabilities TextDocumentSyncClientCapabilities_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(TextDocumentSyncClientCapabilities);
    }
    TextDocumentSyncClientCapabilities value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "dynamicRegistration");
        value.dynamicRegistration = Bool_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "willSave");
        value.willSave = Bool_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "willSaveWaitUntil");
        value.willSaveWaitUntil = Bool_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "didSave");
        value.didSave = Bool_decode(v0);
    }
    RETURN_VALUE(TextDocumentSyncClientCapabilities, value);
}

OptionalJSONValue TextDocumentSyncClientCapabilities_encode(TextDocumentSyncClientCapabilities value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.dynamicRegistration.has_value) {
            _encoded_maybe = Bool_encode(value.dynamicRegistration.value);
        }
        json_optional_set(&v1, "dynamicRegistration", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.willSave.has_value) {
            _encoded_maybe = Bool_encode(value.willSave.value);
        }
        json_optional_set(&v1, "willSave", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.willSaveWaitUntil.has_value) {
            _encoded_maybe = Bool_encode(value.willSaveWaitUntil.value);
        }
        json_optional_set(&v1, "willSaveWaitUntil", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.didSave.has_value) {
            _encoded_maybe = Bool_encode(value.didSave.value);
        }
        json_optional_set(&v1, "didSave", _encoded_maybe);
    }
    RETURN_VALUE(JSONValue, v1);
}
