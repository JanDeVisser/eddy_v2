/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/TextDocumentSyncOptions.h>

DA_IMPL(TextDocumentSyncOptions)

OptionalJSONValue OptionalTextDocumentSyncOptions_encode(OptionalTextDocumentSyncOptions value)
{
    if (value.has_value) {
        return TextDocumentSyncOptions_encode(value.value);
    } else {
        RETURN_EMPTY(JSONValue);
    }
}

OptionalOptionalTextDocumentSyncOptions OptionalTextDocumentSyncOptions_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(OptionalTextDocumentSyncOptions);
    }
    RETURN_VALUE(OptionalTextDocumentSyncOptions, TextDocumentSyncOptions_decode(json));
}

OptionalJSONValue TextDocumentSyncOptionss_encode(TextDocumentSyncOptionss value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, TextDocumentSyncOptions_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalTextDocumentSyncOptionss TextDocumentSyncOptionss_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(TextDocumentSyncOptionss);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    TextDocumentSyncOptionss ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue               elem = json_at(&json.value, ix);
        OptionalTextDocumentSyncOptions val = TextDocumentSyncOptions_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(TextDocumentSyncOptionss);
        }
        da_append_TextDocumentSyncOptions(&ret, val.value);
    }
    RETURN_VALUE(TextDocumentSyncOptionss, ret);
}
OptionalTextDocumentSyncOptions TextDocumentSyncOptions_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(TextDocumentSyncOptions);
    }
    TextDocumentSyncOptions value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "openClose");
        value.openClose = FORWARD_OPTIONAL(OptionalBool, TextDocumentSyncOptions, OptionalBool_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "change");
        value.change = FORWARD_OPTIONAL(OptionalTextDocumentSyncKind, TextDocumentSyncOptions, OptionalTextDocumentSyncKind_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "willSave");
        value.willSave = FORWARD_OPTIONAL(OptionalBool, TextDocumentSyncOptions, OptionalBool_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "willSaveWaitUntil");
        value.willSaveWaitUntil = FORWARD_OPTIONAL(OptionalBool, TextDocumentSyncOptions, OptionalBool_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "save");
        if (v0.has_value) {
            value.save.has_value = true;
            while (true) {
                {
                    OptionalBool decoded = Bool_decode(v0);
                    if (decoded.has_value) {
                        value.save.tag = 0;
                        value.save._0 = decoded.value;
                        break;
                    }
                }
                {
                    OptionalSaveOptions decoded = SaveOptions_decode(v0);
                    if (decoded.has_value) {
                        value.save.tag = 1;
                        value.save._1 = decoded.value;
                        break;
                    }
                }
                RETURN_EMPTY(TextDocumentSyncOptions);
            }
        }
    }
    RETURN_VALUE(TextDocumentSyncOptions, value);
}
OptionalJSONValue TextDocumentSyncOptions_encode(TextDocumentSyncOptions value)
{
    JSONValue v1 = json_object();
    json_optional_set(&v1, "openClose", OptionalBool_encode(value.openClose));
    json_optional_set(&v1, "change", OptionalTextDocumentSyncKind_encode(value.change));
    json_optional_set(&v1, "willSave", OptionalBool_encode(value.willSave));
    json_optional_set(&v1, "willSaveWaitUntil", OptionalBool_encode(value.willSaveWaitUntil));
    if (value.save.has_value) {
        JSONValue v2 = { 0 };
        switch (value.save.tag) {
        case 0:
            v2 = MUST_OPTIONAL(JSONValue, Bool_encode(value.save._0));

            break;
        case 1:
            v2 = MUST_OPTIONAL(JSONValue, SaveOptions_encode(value.save._1));

            break;
        default:
            UNREACHABLE();
        }

        json_set(&v1, "save", v2);
    }

    RETURN_VALUE(JSONValue, v1);
}
