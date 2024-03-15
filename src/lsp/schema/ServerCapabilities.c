/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/ServerCapabilities.h>

DA_IMPL(ServerCapabilities)

OptionalJSONValue OptionalServerCapabilities_encode(OptionalServerCapabilities value)
{
    if (value.has_value) {
        return ServerCapabilities_encode(value.value);
    } else {
        RETURN_EMPTY(JSONValue);
    }
}

OptionalOptionalServerCapabilities OptionalServerCapabilities_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(OptionalServerCapabilities);
    }
    RETURN_VALUE(OptionalServerCapabilities, ServerCapabilities_decode(json));
}

OptionalJSONValue ServerCapabilitiess_encode(ServerCapabilitiess value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, ServerCapabilities_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalServerCapabilitiess ServerCapabilitiess_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(ServerCapabilitiess);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    ServerCapabilitiess ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue          elem = json_at(&json.value, ix);
        OptionalServerCapabilities val = ServerCapabilities_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(ServerCapabilitiess);
        }
        da_append_ServerCapabilities(&ret, val.value);
    }
    RETURN_VALUE(ServerCapabilitiess, ret);
}
OptionalServerCapabilities ServerCapabilities_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(ServerCapabilities);
    }
    ServerCapabilities value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "positionEncoding");
        value.positionEncoding = FORWARD_OPTIONAL(OptionalPositionEncodingKind, ServerCapabilities, OptionalPositionEncodingKind_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "textDocumentSync");
        if (v0.has_value) {
            value.textDocumentSync.has_value = true;
            while (true) {
                {
                    OptionalTextDocumentSyncOptions decoded = TextDocumentSyncOptions_decode(v0);
                    if (decoded.has_value) {
                        value.textDocumentSync.tag = 0;
                        value.textDocumentSync._0 = decoded.value;
                        break;
                    }
                }
                {
                    OptionalTextDocumentSyncKind decoded = TextDocumentSyncKind_decode(v0);
                    if (decoded.has_value) {
                        value.textDocumentSync.tag = 1;
                        value.textDocumentSync._1 = decoded.value;
                        break;
                    }
                }
                RETURN_EMPTY(ServerCapabilities);
            }
        }
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "semanticTokensProvider");
        value.semanticTokensProvider = FORWARD_OPTIONAL(OptionalSemanticTokensOptions, ServerCapabilities, OptionalSemanticTokensOptions_decode(v0));
    }
    RETURN_VALUE(ServerCapabilities, value);
}
OptionalJSONValue ServerCapabilities_encode(ServerCapabilities value)
{
    JSONValue v1 = json_object();
    json_optional_set(&v1, "positionEncoding", OptionalPositionEncodingKind_encode(value.positionEncoding));
    if (value.textDocumentSync.has_value) {
        JSONValue v2 = { 0 };
        switch (value.textDocumentSync.tag) {
        case 0:
            v2 = MUST_OPTIONAL(JSONValue, TextDocumentSyncOptions_encode(value.textDocumentSync._0));

            break;
        case 1:
            v2 = MUST_OPTIONAL(JSONValue, TextDocumentSyncKind_encode(value.textDocumentSync._1));

            break;
        default:
            UNREACHABLE();
        }

        json_set(&v1, "textDocumentSync", v2);
    }

    json_optional_set(&v1, "semanticTokensProvider", OptionalSemanticTokensOptions_encode(value.semanticTokensProvider));
    RETURN_VALUE(JSONValue, v1);
}
