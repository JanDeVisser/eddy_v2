/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/SemanticTokenModifiers.h>

DA_IMPL(SemanticTokenModifiers)

OptionalJSONValue SemanticTokenModifierss_encode(SemanticTokenModifierss value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, SemanticTokenModifiers_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalSemanticTokenModifierss SemanticTokenModifierss_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(SemanticTokenModifierss);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    SemanticTokenModifierss ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue              elem = json_at(&json.value, ix);
        OptionalSemanticTokenModifiers val = SemanticTokenModifiers_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(SemanticTokenModifierss);
        }
        da_append_SemanticTokenModifiers(&ret, val.value);
    }
    RETURN_VALUE(SemanticTokenModifierss, ret);
}

StringView SemanticTokenModifiers_to_string(SemanticTokenModifiers value)
{
    switch (value) {
    case SemanticTokenModifiersDeclaration:
        return sv_from("declaration");
    case SemanticTokenModifiersDefinition:
        return sv_from("definition");
    case SemanticTokenModifiersReadonly:
        return sv_from("readonly");
    case SemanticTokenModifiersStatic:
        return sv_from("static");
    case SemanticTokenModifiersDeprecated:
        return sv_from("deprecated");
    case SemanticTokenModifiersAbstract:
        return sv_from("abstract");
    case SemanticTokenModifiersAsync:
        return sv_from("async");
    case SemanticTokenModifiersModification:
        return sv_from("modification");
    case SemanticTokenModifiersDocumentation:
        return sv_from("documentation");
    case SemanticTokenModifiersDefaultLibrary:
        return sv_from("defaultLibrary");
    default:
        UNREACHABLE();
    }
}

OptionalSemanticTokenModifiers SemanticTokenModifiers_parse(StringView s)
{
    if (sv_eq_cstr(s, "declaration"))
        RETURN_VALUE(SemanticTokenModifiers, SemanticTokenModifiersDeclaration);
    if (sv_eq_cstr(s, "definition"))
        RETURN_VALUE(SemanticTokenModifiers, SemanticTokenModifiersDefinition);
    if (sv_eq_cstr(s, "readonly"))
        RETURN_VALUE(SemanticTokenModifiers, SemanticTokenModifiersReadonly);
    if (sv_eq_cstr(s, "static"))
        RETURN_VALUE(SemanticTokenModifiers, SemanticTokenModifiersStatic);
    if (sv_eq_cstr(s, "deprecated"))
        RETURN_VALUE(SemanticTokenModifiers, SemanticTokenModifiersDeprecated);
    if (sv_eq_cstr(s, "abstract"))
        RETURN_VALUE(SemanticTokenModifiers, SemanticTokenModifiersAbstract);
    if (sv_eq_cstr(s, "async"))
        RETURN_VALUE(SemanticTokenModifiers, SemanticTokenModifiersAsync);
    if (sv_eq_cstr(s, "modification"))
        RETURN_VALUE(SemanticTokenModifiers, SemanticTokenModifiersModification);
    if (sv_eq_cstr(s, "documentation"))
        RETURN_VALUE(SemanticTokenModifiers, SemanticTokenModifiersDocumentation);
    if (sv_eq_cstr(s, "defaultLibrary"))
        RETURN_VALUE(SemanticTokenModifiers, SemanticTokenModifiersDefaultLibrary);
    RETURN_EMPTY(SemanticTokenModifiers);
}

OptionalSemanticTokenModifiers SemanticTokenModifiers_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(SemanticTokenModifiers);
    }
    assert(json.value.type == JSON_TYPE_STRING);
    return SemanticTokenModifiers_parse(json.value.string);
}

OptionalJSONValue SemanticTokenModifiers_encode(SemanticTokenModifiers value)
{
    RETURN_VALUE(JSONValue, json_string(SemanticTokenModifiers_to_string(value)));
}
