/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/SemanticTokenTypes.h>

DA_IMPL(SemanticTokenTypes)

OptionalJSONValue OptionalSemanticTokenTypes_encode(OptionalSemanticTokenTypes value)
{
    if (value.has_value) {
        return SemanticTokenTypes_encode(value.value);
    } else {
        RETURN_EMPTY(JSONValue);
    }
}

OptionalOptionalSemanticTokenTypes OptionalSemanticTokenTypes_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(OptionalSemanticTokenTypes);
    }
    RETURN_VALUE(OptionalSemanticTokenTypes, SemanticTokenTypes_decode(json));
}

OptionalJSONValue SemanticTokenTypess_encode(SemanticTokenTypess value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, SemanticTokenTypes_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalSemanticTokenTypess SemanticTokenTypess_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(SemanticTokenTypess);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    SemanticTokenTypess ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue          elem = json_at(&json.value, ix);
        OptionalSemanticTokenTypes val = SemanticTokenTypes_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(SemanticTokenTypess);
        }
        da_append_SemanticTokenTypes(&ret, val.value);
    }
    RETURN_VALUE(SemanticTokenTypess, ret);
}
StringView SemanticTokenTypes_to_string(SemanticTokenTypes value)
{
    switch (value) {
    case SemanticTokenTypesNamespace:
        return sv_from("namespace");
    case SemanticTokenTypesType:
        return sv_from("type");
    case SemanticTokenTypesClass:
        return sv_from("class");
    case SemanticTokenTypesEnum:
        return sv_from("enum");
    case SemanticTokenTypesInterface:
        return sv_from("interface");
    case SemanticTokenTypesStruct:
        return sv_from("struct");
    case SemanticTokenTypesTypeParameter:
        return sv_from("typeParameter");
    case SemanticTokenTypesParameter:
        return sv_from("parameter");
    case SemanticTokenTypesVariable:
        return sv_from("variable");
    case SemanticTokenTypesProperty:
        return sv_from("property");
    case SemanticTokenTypesEnumMember:
        return sv_from("enumMember");
    case SemanticTokenTypesEvent:
        return sv_from("event");
    case SemanticTokenTypesFunction:
        return sv_from("function");
    case SemanticTokenTypesMethod:
        return sv_from("method");
    case SemanticTokenTypesMacro:
        return sv_from("macro");
    case SemanticTokenTypesKeyword:
        return sv_from("keyword");
    case SemanticTokenTypesModifier:
        return sv_from("modifier");
    case SemanticTokenTypesComment:
        return sv_from("comment");
    case SemanticTokenTypesString:
        return sv_from("string");
    case SemanticTokenTypesNumber:
        return sv_from("number");
    case SemanticTokenTypesRegexp:
        return sv_from("regexp");
    case SemanticTokenTypesOperator:
        return sv_from("operator");
    case SemanticTokenTypesDecorator:
        return sv_from("decorator");
    default:
        UNREACHABLE();
    }
}

OptionalSemanticTokenTypes SemanticTokenTypes_parse(StringView s)
{
    if (sv_eq_cstr(s, "namespace"))
        RETURN_VALUE(SemanticTokenTypes, SemanticTokenTypesNamespace);
    if (sv_eq_cstr(s, "type"))
        RETURN_VALUE(SemanticTokenTypes, SemanticTokenTypesType);
    if (sv_eq_cstr(s, "class"))
        RETURN_VALUE(SemanticTokenTypes, SemanticTokenTypesClass);
    if (sv_eq_cstr(s, "enum"))
        RETURN_VALUE(SemanticTokenTypes, SemanticTokenTypesEnum);
    if (sv_eq_cstr(s, "interface"))
        RETURN_VALUE(SemanticTokenTypes, SemanticTokenTypesInterface);
    if (sv_eq_cstr(s, "struct"))
        RETURN_VALUE(SemanticTokenTypes, SemanticTokenTypesStruct);
    if (sv_eq_cstr(s, "typeParameter"))
        RETURN_VALUE(SemanticTokenTypes, SemanticTokenTypesTypeParameter);
    if (sv_eq_cstr(s, "parameter"))
        RETURN_VALUE(SemanticTokenTypes, SemanticTokenTypesParameter);
    if (sv_eq_cstr(s, "variable"))
        RETURN_VALUE(SemanticTokenTypes, SemanticTokenTypesVariable);
    if (sv_eq_cstr(s, "property"))
        RETURN_VALUE(SemanticTokenTypes, SemanticTokenTypesProperty);
    if (sv_eq_cstr(s, "enumMember"))
        RETURN_VALUE(SemanticTokenTypes, SemanticTokenTypesEnumMember);
    if (sv_eq_cstr(s, "event"))
        RETURN_VALUE(SemanticTokenTypes, SemanticTokenTypesEvent);
    if (sv_eq_cstr(s, "function"))
        RETURN_VALUE(SemanticTokenTypes, SemanticTokenTypesFunction);
    if (sv_eq_cstr(s, "method"))
        RETURN_VALUE(SemanticTokenTypes, SemanticTokenTypesMethod);
    if (sv_eq_cstr(s, "macro"))
        RETURN_VALUE(SemanticTokenTypes, SemanticTokenTypesMacro);
    if (sv_eq_cstr(s, "keyword"))
        RETURN_VALUE(SemanticTokenTypes, SemanticTokenTypesKeyword);
    if (sv_eq_cstr(s, "modifier"))
        RETURN_VALUE(SemanticTokenTypes, SemanticTokenTypesModifier);
    if (sv_eq_cstr(s, "comment"))
        RETURN_VALUE(SemanticTokenTypes, SemanticTokenTypesComment);
    if (sv_eq_cstr(s, "string"))
        RETURN_VALUE(SemanticTokenTypes, SemanticTokenTypesString);
    if (sv_eq_cstr(s, "number"))
        RETURN_VALUE(SemanticTokenTypes, SemanticTokenTypesNumber);
    if (sv_eq_cstr(s, "regexp"))
        RETURN_VALUE(SemanticTokenTypes, SemanticTokenTypesRegexp);
    if (sv_eq_cstr(s, "operator"))
        RETURN_VALUE(SemanticTokenTypes, SemanticTokenTypesOperator);
    if (sv_eq_cstr(s, "decorator"))
        RETURN_VALUE(SemanticTokenTypes, SemanticTokenTypesDecorator);
    RETURN_EMPTY(SemanticTokenTypes);
}
OptionalSemanticTokenTypes SemanticTokenTypes_decode(OptionalJSONValue json)
{
    assert(json.has_value);
    assert(json.value.type == JSON_TYPE_STRING);
    return SemanticTokenTypes_parse(json.value.string);
}
OptionalJSONValue SemanticTokenTypes_encode(SemanticTokenTypes value)
{
    RETURN_VALUE(JSONValue, json_string(SemanticTokenTypes_to_string(value)));
}
