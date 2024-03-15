/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/SemanticTokensClientCapabilities.h>

DA_IMPL(SemanticTokensClientCapabilities)

OptionalJSONValue OptionalSemanticTokensClientCapabilities_encode(OptionalSemanticTokensClientCapabilities value)
{
    if (value.has_value) {
        return SemanticTokensClientCapabilities_encode(value.value);
    } else {
        RETURN_EMPTY(JSONValue);
    }
}

OptionalOptionalSemanticTokensClientCapabilities OptionalSemanticTokensClientCapabilities_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(OptionalSemanticTokensClientCapabilities);
    }
    RETURN_VALUE(OptionalSemanticTokensClientCapabilities, SemanticTokensClientCapabilities_decode(json));
}

OptionalJSONValue SemanticTokensClientCapabilitiess_encode(SemanticTokensClientCapabilitiess value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, SemanticTokensClientCapabilities_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalSemanticTokensClientCapabilitiess SemanticTokensClientCapabilitiess_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(SemanticTokensClientCapabilitiess);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    SemanticTokensClientCapabilitiess ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue                        elem = json_at(&json.value, ix);
        OptionalSemanticTokensClientCapabilities val = SemanticTokensClientCapabilities_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(SemanticTokensClientCapabilitiess);
        }
        da_append_SemanticTokensClientCapabilities(&ret, val.value);
    }
    RETURN_VALUE(SemanticTokensClientCapabilitiess, ret);
}
OptionalSemanticTokensClientCapabilities SemanticTokensClientCapabilities_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(SemanticTokensClientCapabilities);
    }
    SemanticTokensClientCapabilities value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "dynamicRegistration");
        value.dynamicRegistration = FORWARD_OPTIONAL(OptionalBool, SemanticTokensClientCapabilities, OptionalBool_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "requests");
        assert(v0.has_value);
        value.requests.has_value = true;
        {
            OptionalJSONValue v1 = json_get(&v0.value, "range");
            if (v1.has_value) {
                value.requests.range.has_value = true;
                while (true) {
                    {
                        OptionalBool decoded = Bool_decode(v1);
                        if (decoded.has_value) {
                            value.requests.range.tag = 0;
                            value.requests.range._0 = decoded.value;
                            break;
                        }
                    }
                    {
                        bool              decoded2 = false;
                        OptionalJSONValue v3 = { 0 };
                        do {
                            decoded2 = true;
                        } while (false);
                        if (decoded2)
                            break;
                    }
                    RETURN_EMPTY(SemanticTokensClientCapabilities);
                }
            }
        }

        {
            OptionalJSONValue v1 = json_get(&v0.value, "full");
            if (v1.has_value) {
                value.requests.full.has_value = true;
                while (true) {
                    {
                        OptionalBool decoded = Bool_decode(v1);
                        if (decoded.has_value) {
                            value.requests.full.tag = 0;
                            value.requests.full._0 = decoded.value;
                            break;
                        }
                    }
                    {
                        bool              decoded2 = false;
                        OptionalJSONValue v3 = { 0 };
                        do {
                            v3 = json_get(&v1.value, "delta");
                            OptionalOptionalBool opt2_1_delta = OptionalBool_decode(v3);
                            if (!opt2_1_delta.has_value) {
                                break;
                            }
                            value.requests.full._1.delta = opt2_1_delta.value;
                            decoded2 = true;
                        } while (false);
                        if (decoded2)
                            break;
                    }
                    RETURN_EMPTY(SemanticTokensClientCapabilities);
                }
            }
        }
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "tokenTypes");
        value.tokenTypes = FORWARD_OPTIONAL(StringViews, SemanticTokensClientCapabilities, StringViews_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "tokenModifiers");
        value.tokenModifiers = FORWARD_OPTIONAL(StringViews, SemanticTokensClientCapabilities, StringViews_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "formats");
        value.formats = FORWARD_OPTIONAL(TokenFormats, SemanticTokensClientCapabilities, TokenFormats_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "overlappingTokenSupport");
        value.overlappingTokenSupport = FORWARD_OPTIONAL(OptionalBool, SemanticTokensClientCapabilities, OptionalBool_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "multilineTokenSupport");
        value.multilineTokenSupport = FORWARD_OPTIONAL(OptionalBool, SemanticTokensClientCapabilities, OptionalBool_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "serverCancelSupport");
        value.serverCancelSupport = FORWARD_OPTIONAL(OptionalBool, SemanticTokensClientCapabilities, OptionalBool_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "augmentsSyntaxTokens");
        value.augmentsSyntaxTokens = FORWARD_OPTIONAL(OptionalBool, SemanticTokensClientCapabilities, OptionalBool_decode(v0));
    }
    RETURN_VALUE(SemanticTokensClientCapabilities, value);
}
OptionalJSONValue SemanticTokensClientCapabilities_encode(SemanticTokensClientCapabilities value)
{
    JSONValue v1 = json_object();
    json_optional_set(&v1, "dynamicRegistration", OptionalBool_encode(value.dynamicRegistration));
    assert(value.requests.has_value);
    {
        JSONValue v2 = json_object();
        if (value.requests.range.has_value) {
            JSONValue v3 = { 0 };
            switch (value.requests.range.tag) {
            case 0:
                v3 = MUST_OPTIONAL(JSONValue, Bool_encode(value.requests.range._0));

                break;
            case 1: {
                v3 = json_object();
            } break;
            default:
                UNREACHABLE();
            }

            json_set(&v2, "range", v3);
        }

        if (value.requests.full.has_value) {
            JSONValue v3 = { 0 };
            switch (value.requests.full.tag) {
            case 0:
                v3 = MUST_OPTIONAL(JSONValue, Bool_encode(value.requests.full._0));

                break;
            case 1: {
                v3 = json_object();
                json_optional_set(&v3, "delta", OptionalBool_encode(value.requests.full._1.delta));
            } break;
            default:
                UNREACHABLE();
            }

            json_set(&v2, "full", v3);
        }

        json_set(&v1, "requests", v2);
    }
    json_optional_set(&v1, "tokenTypes", StringViews_encode(value.tokenTypes));
    json_optional_set(&v1, "tokenModifiers", StringViews_encode(value.tokenModifiers));
    json_optional_set(&v1, "formats", TokenFormats_encode(value.formats));
    json_optional_set(&v1, "overlappingTokenSupport", OptionalBool_encode(value.overlappingTokenSupport));
    json_optional_set(&v1, "multilineTokenSupport", OptionalBool_encode(value.multilineTokenSupport));
    json_optional_set(&v1, "serverCancelSupport", OptionalBool_encode(value.serverCancelSupport));
    json_optional_set(&v1, "augmentsSyntaxTokens", OptionalBool_encode(value.augmentsSyntaxTokens));
    RETURN_VALUE(JSONValue, v1);
}
