#include "json.h"
/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/SemanticTokensOptions.h>

DA_IMPL(SemanticTokensOptions)

OptionalJSONValue SemanticTokensOptionss_encode(SemanticTokensOptionss value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, SemanticTokensOptions_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalSemanticTokensOptionss SemanticTokensOptionss_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(SemanticTokensOptionss);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    SemanticTokensOptionss ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue             elem = json_at(&json.value, ix);
        OptionalSemanticTokensOptions val = SemanticTokensOptions_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(SemanticTokensOptionss);
        }
        da_append_SemanticTokensOptions(&ret, val.value);
    }
    RETURN_VALUE(SemanticTokensOptionss, ret);
}

OptionalSemanticTokensOptions SemanticTokensOptions_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(SemanticTokensOptions);
    }
    SemanticTokensOptions value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "legend");
        value.legend = FORWARD_OPTIONAL(SemanticTokensLegend, SemanticTokensOptions, SemanticTokensLegend_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "range");
        if (v0.has_value) {
            value.range.has_value = true;
            while (true) {
                {
                    OptionalBool decoded = Bool_decode(v0);
                    if (decoded.has_value) {
                        value.range.tag = 0;
                        value.range._0 = decoded.value;
                        break;
                    }
                }
                {
                    bool              decoded1 = false;
                    OptionalJSONValue v2 = { 0 };
                    do {
                        decoded1 = true;
                    } while (false);
                    if (decoded1)
                        break;
                }
                RETURN_EMPTY(SemanticTokensOptions);
            }
        }
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "full");
        if (v0.has_value) {
            value.full.has_value = true;
            while (true) {
                {
                    OptionalBool decoded = Bool_decode(v0);
                    if (decoded.has_value) {
                        value.full.tag = 0;
                        value.full._0 = decoded.value;
                        break;
                    }
                }
                {
                    bool              decoded1 = false;
                    OptionalJSONValue v2 = { 0 };
                    do {
                        v2 = json_get(&v0.value, "delta");
                        value.full._1.delta = Bool_decode(v2);
                        decoded1 = true;
                    } while (false);
                    if (decoded1)
                        break;
                }
                RETURN_EMPTY(SemanticTokensOptions);
            }
        }
    }
    RETURN_VALUE(SemanticTokensOptions, value);
}

OptionalJSONValue SemanticTokensOptions_encode(SemanticTokensOptions value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = SemanticTokensLegend_encode(value.legend);
        json_optional_set(&v1, "legend", _encoded_maybe);
    }
    if (value.range.has_value) {
        JSONValue v2 = { 0 };
        switch (value.range.tag) {
        case 0:
            v2 = MUST_OPTIONAL(JSONValue, Bool_encode(value.range._0));

            break;
        case 1: {
            v2 = json_object();
        } break;
        default:
            UNREACHABLE();
        }

        json_set(&v1, "range", v2);
    }

    if (value.full.has_value) {
        JSONValue v2 = { 0 };
        switch (value.full.tag) {
        case 0:
            v2 = MUST_OPTIONAL(JSONValue, Bool_encode(value.full._0));

            break;
        case 1: {
            v2 = json_object();
            {
                OptionalJSONValue _encoded_maybe = { 0 };
                if (value.full._1.delta.has_value) {
                    _encoded_maybe = Bool_encode(value.full._1.delta.value);
                }
                json_optional_set(&v2, "delta", _encoded_maybe);
            }
        } break;
        default:
            UNREACHABLE();
        }

        json_set(&v1, "full", v2);
    }

    RETURN_VALUE(JSONValue, v1);
}
