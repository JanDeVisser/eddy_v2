/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/TextDocumentSyncClientCapabilities.h>

DA_IMPL(TextDocumentSyncClientCapabilities)

OptionalJSONValue OptionalTextDocumentSyncClientCapabilities_encode(OptionalTextDocumentSyncClientCapabilities value)
{
    if (value.has_value) {
        return TextDocumentSyncClientCapabilities_encode(value.value);
    } else {
        RETURN_EMPTY(JSONValue);
    }
}

OptionalOptionalTextDocumentSyncClientCapabilities OptionalTextDocumentSyncClientCapabilities_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(OptionalTextDocumentSyncClientCapabilities);
    }
    RETURN_VALUE(OptionalTextDocumentSyncClientCapabilities, TextDocumentSyncClientCapabilities_decode(json));
}

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
        value.dynamicRegistration = FORWARD_OPTIONAL(OptionalBool, TextDocumentSyncClientCapabilities, OptionalBool_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "willSave");
        value.willSave = FORWARD_OPTIONAL(OptionalBool, TextDocumentSyncClientCapabilities, OptionalBool_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "willSaveWaitUntil");
        value.willSaveWaitUntil = FORWARD_OPTIONAL(OptionalBool, TextDocumentSyncClientCapabilities, OptionalBool_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "didSave");
        value.didSave = FORWARD_OPTIONAL(OptionalBool, TextDocumentSyncClientCapabilities, OptionalBool_decode(v0));
    }
    RETURN_VALUE(TextDocumentSyncClientCapabilities, value);
}
OptionalJSONValue TextDocumentSyncClientCapabilities_encode(TextDocumentSyncClientCapabilities value)
{
    JSONValue v1 = json_object();
    json_optional_set(&v1, "dynamicRegistration", OptionalBool_encode(value.dynamicRegistration));
    json_optional_set(&v1, "willSave", OptionalBool_encode(value.willSave));
    json_optional_set(&v1, "willSaveWaitUntil", OptionalBool_encode(value.willSaveWaitUntil));
    json_optional_set(&v1, "didSave", OptionalBool_encode(value.didSave));
    RETURN_VALUE(JSONValue, v1);
}
