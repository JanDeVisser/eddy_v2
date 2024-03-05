/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <io.h>
#include <lexer.h>
#include <template/template.h>
#include <ts.h>

typedef enum {
    TSKeywordBoolean,
    TSKeywordConst,
    TSKeywordDecimal,
    TSKeywordDocumentURI,
    TSKeywordExport,
    TSKeywordExtends,
    TSKeywordInteger,
    TSKeywordInterface,
    TSKeywordLSPAny,
    TSKeywordNamespace,
    TSKeywordNull,
    TSKeywordString,
    TSKeywordType,
    TSKeywordUInteger,
    TSKeywordURI,
} TSKeywords;

static Keyword ts_keywords[] = {
    { "boolean", TSKeywordBoolean },
    { "const", TSKeywordConst },
    { "decimal", TSKeywordDecimal },
    { "DocumentURI", TSKeywordDocumentURI },
    { "export", TSKeywordExport },
    { "extends", TSKeywordExtends },
    { "integer", TSKeywordInteger },
    { "interface", TSKeywordInterface },
    { "LSPAny", TSKeywordLSPAny },
    { "namespace", TSKeywordNamespace },
    { "null", TSKeywordNull },
    { "string", TSKeywordString },
    { "type", TSKeywordType },
    { "uinteger", TSKeywordUInteger },
    { "URI", TSKeywordURI },
    { NULL, 0 }
};

static Language ts_language = {
    .name = (StringView) { .ptr = "TypeScript", .length = 10 },
    .directives = NULL,
    .preprocessor_trigger = (Token) { 0 },
    .keywords = ts_keywords,
    .directive_handler = NULL,
};

typedef enum {
    TypeKindNone = 0,
    TypeKindBasic,
    TypeKindConstant,
    TypeKindType,
    TypeKindAnonymousVariant,
    TypeKindAnonymousStruct,
} TypeKind;

typedef enum {
    BasicTypeNone = 0,
    BasicTypeAny,
    BasicTypeBool,
    BasicTypeInt,
    BasicTypeNull,
    BasicTypeString,
    BasicTypeUnsigned,
} BasicType;

typedef struct {
    StringView name;
    union {
        StringView string_value;
        int        int_value;
    };
} NamespaceValue;

DA_WITH_NAME(NamespaceValue, NamespaceValues);

typedef struct {
    StringView      name;
    BasicType       value_type;
    NamespaceValues values;
} Namespace;

typedef struct {
    StringView name;
    BasicType  type;
    union {
        StringView string_value;
        int        int_value;
        bool       bool_value;
    };
} ConstantType;

DA_WITH_NAME(Namespace, Namespaces);

typedef struct _da_Property Properties;
typedef struct anon_variant Variant;

typedef struct {
    TypeKind kind;
    bool     array;
    union {
        StringView   name;
        BasicType    basic_type;
        ConstantType constant;
        Properties  *anon_struct;
        Variant     *anon_variant;
    };
} Type;

DA_WITH_NAME(Type, Types);

typedef struct {
    StringView name;
    bool       optional;
    Type       type;
} Property;

DA_WITH_NAME(Property, Properties);

typedef struct anon_variant {
    Types options;
} Variant;

typedef struct {
    StringList extends;
    Properties properties;
} Interface;

DA_WITH_NAME(Interface, Interfaces);

typedef enum {
    TypeDefKindNone = 0,
    TypeDefKindAlias,
    TypeDefKindInterface,
} TypeDefKind;

typedef struct {
    TypeDefKind kind;
    StringView  name;
    union {
        Type      alias_for;
        Interface interface;
    };
} TypeDef;

DA_WITH_NAME(TypeDef, TypeDefs);

typedef struct {
    StringView name;
    StringList types;
} Module;

DA_WITH_NAME(Module, Modules);

DA_IMPL(Type);
DA_IMPL(Property);
DA_IMPL(Interface);
DA_IMPL(NamespaceValue);
DA_IMPL(Namespace);
DA_IMPL(TypeDef);
DA_IMPL(Module);

static TypeDef *get_typedef(StringView name);
static TypeDef *get_interface(StringView name);
char const     *basic_type_name(BasicType basic_type);
StringView      constant_to_string(ConstantType constant);
StringView      property_to_string(Property property);
StringView      properties_to_string(Properties properties);
StringView      namespace_to_string(Namespace namespace);
StringView      type_to_string(Type type);
StringView      typedef_to_string(TypeDef type_def);
StringView      interface_to_string(Interface interface);
JSONValue       interface_serialize(Interface interface);
JSONValue       typedef_serialize(TypeDef type_def);
JSONValue       module_serialize(Module module);

static void parse_struct(Lexer *lexer, Properties *s);
static Type parse_type(Lexer *lexer);
void        parse_interface(Lexer *lexer);
void        parse_interface(Lexer *lexer);
void        parse_typedef(Lexer *lexer);

static Namespaces namespaces = { 0 };
static TypeDefs   typedefs = { 0 };
static Modules    modules = { 0 };

TypeDef *get_typedef(StringView name)
{
    for (size_t ix = 0; ix < typedefs.size; ++ix) {
        if (sv_eq(name, typedefs.elements[ix].name)) {
            return typedefs.elements + ix;
        }
    }
    return NULL;
}

char const *basic_type_name(BasicType basic_type)
{
    switch (basic_type) {
    case BasicTypeAny:
        return "LSPAny";
    case BasicTypeBool:
        return "boolean";
    case BasicTypeInt:
        return "integer";
    case BasicTypeNull:
        return "null";
    case BasicTypeString:
        return "string";
    case BasicTypeUnsigned:
        return "uinteger";
    default:
        UNREACHABLE();
    }
}

StringView constant_to_string(ConstantType constant)
{
    switch (constant.type) {
    case BasicTypeString:
        return sv_copy(constant.string_value);
    case BasicTypeInt:
    case BasicTypeUnsigned:
        return sv_printf("%d", constant.int_value);
    case BasicTypeNull:
        return sv_copy_cstr("null");
    case BasicTypeBool:
        return sv_copy_cstr(constant.bool_value ? "true" : "false");
    default:
        UNREACHABLE();
    }
}

StringView property_to_string(Property property)
{
    StringBuilder ret = { 0 };
    StringView    t = type_to_string(property.type);
    char const   *o = (property.optional) ? "?" : "";
    sb_printf(&ret, "%.*s%s: %.*s", SV_ARG(property.name), o, SV_ARG(t));
    sv_free(t);
    return ret.view;
}

StringView properties_to_string(Properties properties)
{
    StringBuilder ret = { 0 };
    StringList    props = { 0 };
    for (size_t ix = 0; ix < properties.size; ++ix) {
        StringView p = property_to_string(properties.elements[ix]);
        sl_push(&props, p);
    }
    sb_append_cstr(&ret, "{\n  ");
    sb_append_list(&ret, &props, sv_from(";\n  "));
    sb_append_cstr(&ret, ";\n}");
    sl_free(&props);
    return ret.view;
}

StringView type_to_string(Type type)
{
    StringBuilder ret = { 0 };
    switch (type.kind) {
    case TypeKindBasic:
        sb_append_cstr(&ret, basic_type_name(type.basic_type));
        break;
    case TypeKindType:
        sb_append_sv(&ret, type.name);
        break;
    case TypeKindConstant: {
        StringView constant = constant_to_string(type.constant);
        sb_append_sv(&ret, constant);
        sv_free(constant);
    } break;
    case TypeKindAnonymousVariant: {
        StringList variants = { 0 };
        for (size_t ix = 0; ix < type.anon_variant->options.size; ++ix) {
            StringView v = type_to_string(type.anon_variant->options.elements[ix]);
            sl_push(&variants, v);
        }
        sb_append_list(&ret, &variants, sv_from(" | "));
    } break;
    case TypeKindAnonymousStruct: {
        StringView props = properties_to_string(*type.anon_struct);
        sb_append_sv(&ret, props);
        sv_free(props);
    } break;
    default:
        UNREACHABLE();
    }
    if (type.array) {
        sb_append_cstr(&ret, "[]");
    }
    return ret.view;
}

StringView interface_to_string(Interface interface)
{
    StringBuilder ret = { 0 };
    if (interface.extends.size > 0) {
        StringView extends = sl_join(&interface.extends, sv_from(", "));
        sb_printf(&ret, "extends %.*s ", SV_ARG(extends));
        sv_free(extends);
    }
    StringView props = properties_to_string(interface.properties);
    sb_append_sv(&ret, props);
    sv_free(props);
    return ret.view;
}

StringView namespace_to_string(Namespace namespace)
{
    StringBuilder ret = sb_createf("namespace %.*s {\n", SV_ARG(namespace.name));
    for (size_t ix = 0; ix < namespace.values.size; ++ix) {
        NamespaceValue *value = namespace.values.elements + ix;
        sb_printf(&ret, "  %.*s: %s =", SV_ARG(value->name), basic_type_name(namespace.value_type));
        switch (namespace.value_type) {
        case BasicTypeInt:
        case BasicTypeUnsigned:
            sb_printf(&ret, "%d", value->int_value);
            break;
        case BasicTypeString:
            sb_append_sv(&ret, value->string_value);
            break;
        default:
            UNREACHABLE();
        }
        sb_append_cstr(&ret, ";\n");
    }
    sb_append_cstr(&ret, "}");
    return ret.view;
}

StringView typedef_to_string(TypeDef type_def)
{
    StringBuilder ret = { 0 };
    switch (type_def.kind) {
    case TypeDefKindAlias: {
        StringView alias = type_to_string(type_def.alias_for);
        sb_printf(&ret, "type %.*s = %.*s;", SV_ARG(type_def.name), SV_ARG(alias));
        sv_free(alias);
    } break;
    case TypeDefKindInterface: {
        StringView iface = interface_to_string(type_def.interface);
        sb_printf(&ret, "interface %.*s %.*s;", SV_ARG(type_def.name), SV_ARG(iface));
        sv_free(iface);
    } break;
    default:
        UNREACHABLE();
    }
    return ret.view;
}

JSONValue constant_serialize(ConstantType constant)
{
    JSONValue ret = json_object();
    json_set_cstr(&ret, "type", basic_type_name(constant.type));
    switch (constant.type) {
    case BasicTypeString:
        json_set_string(&ret, basic_type_name(constant.type), constant.string_value);
        break;
    case BasicTypeInt:
    case BasicTypeUnsigned:
        json_set_int(&ret, basic_type_name(constant.type), constant.int_value);
        break;
    case BasicTypeBool:
        json_set(&ret, basic_type_name(constant.type), json_bool(constant.bool_value));
        break;
    case BasicTypeNull:
        break;
    default:
        fatal("Cannot serialize constants with basic type %s", basic_type_name(constant.type));
    }
    return ret;
}

JSONValue type_serialize(Type type)
{
    JSONValue ret = json_object();
    switch (type.kind) {
    case TypeKindBasic:
        json_set_cstr(&ret, "kind", "basic_type");
        json_set_cstr(&ret, "basic_type", basic_type_name(type.basic_type));
        break;
    case TypeKindType:
        json_set_cstr(&ret, "kind", "typeref");
        json_set_string(&ret, "typeref", type.name);
        break;
    case TypeKindConstant: {
        json_set_cstr(&ret, "kind", "constant");
        json_set(&ret, "constant", constant_serialize(type.constant));
    } break;
    case TypeKindAnonymousVariant: {
        json_set_cstr(&ret, "kind", "variant");
        JSONValue options = json_array();
        for (size_t ix = 0; ix < type.anon_variant->options.size; ++ix) {
            json_append(&options, type_serialize(type.anon_variant->options.elements[ix]));
        }
        json_set(&ret, "options", options);
    } break;
    case TypeKindAnonymousStruct: {
        json_set_cstr(&ret, "kind", "struct");
        JSONValue properties = json_array();
        for (size_t ix = 0; ix < type.anon_struct->size; ++ix) {
            JSONValue prop = json_object();
            Property *p = type.anon_struct->elements + ix;
            json_set(&prop, "name", json_string(p->name));
            json_set(&prop, "optional", json_bool(p->optional));
            json_set(&prop, "type", type_serialize(p->type));
            json_append(&properties, prop);
        }
        json_set(&ret, "properties", properties);
    } break;
    default:
        UNREACHABLE();
    }
    json_set(&ret, "array", json_bool(type.array));
    return ret;
}

void add_properties(JSONValue *properties, Interface interface)
{
    for (size_t ix = 0; ix < interface.extends.size; ++ix) {
        TypeDef *type_def = get_typedef(interface.extends.strings[ix]);
        assert(type_def != NULL);
        assert(type_def->kind == TypeDefKindInterface);
        Interface base = type_def->interface;
        add_properties(properties, base);
    }
    for (size_t ix = 0; ix < interface.properties.size; ++ix) {
        JSONValue prop = json_object();
        Property *p = interface.properties.elements + ix;
        json_set(&prop, "name", json_string(p->name));
        json_set(&prop, "optional", json_bool(p->optional));
        json_set(&prop, "type", type_serialize(p->type));
        json_append(properties, prop);
    }
}

JSONValue interface_serialize(Interface interface)
{
    JSONValue ret = json_object();
    JSONValue extends = json_array();
    for (size_t ix = 0; ix < interface.extends.size; ++ix) {
        json_append(&extends, json_string(interface.extends.strings[ix]));
    }
    json_set(&ret, "extends", extends);
    JSONValue properties = json_array();
    add_properties(&properties, interface);
    json_set(&ret, "properties", properties);
    return ret;
}

JSONValue namespace_serialize(Namespace namespace)
{
    JSONValue ret = json_object();
    json_set_string(&ret, "name", namespace.name);
    JSONValue values = json_array();
    for (size_t ix = 0; ix < namespace.values.size; ++ix) {
        NamespaceValue *value = namespace.values.elements + ix;
        JSONValue       val = json_object();
        json_set_string(&val, "name", value->name);
        json_set_cstr(&val, "type", basic_type_name(namespace.value_type));
        switch (namespace.value_type) {
        case BasicTypeInt:
        case BasicTypeUnsigned:
            json_set_int(&val, basic_type_name(namespace.value_type), value->int_value);
            break;
        case BasicTypeString:
            json_set_string(&val, basic_type_name(namespace.value_type), value->string_value);
            break;
        default:
            UNREACHABLE();
        }
    }
    return ret;
}

JSONValue typedef_serialize(TypeDef type_def)
{
    JSONValue ret = json_object();
    json_set_string(&ret, "name", type_def.name);
    switch (type_def.kind) {
    case TypeDefKindAlias: {
        json_set_cstr(&ret, "kind", "alias");
        JSONValue alias_for = type_serialize(type_def.alias_for);
        json_set(&ret, "alias_for", alias_for);
    } break;
    case TypeDefKindInterface: {
        json_set_cstr(&ret, "kind", "interface");
        JSONValue interface = interface_serialize(type_def.interface);
        json_set(&ret, "interface", interface);
    } break;
    default:
        UNREACHABLE();
    }
    return ret;
}

JSONValue module_serialize(Module module)
{
    JSONValue ret = json_object();
    json_set_string(&ret, "name", module.name);
    JSONValue types_array = json_array();
    for (size_t ix = 0; ix < module.types.size; ++ix) {
        StringView name = module.types.strings[ix];
        TypeDef   *type_def = get_typedef(name);
        assert(type_def != NULL);
        json_append(&types_array, typedef_serialize(*type_def));
    }
    json_set(&ret, "types", types_array);
    return ret;
}

Type parse_type(Lexer *lexer)
{
    Type ret = { 0 };
    while (true) {
        Token t = lexer_lex(lexer);
        Type  type = { 0 };
        switch (t.kind) {
        case TK_SYMBOL: {
            switch (t.code) {
            case '{': {
                type.kind = TypeKindAnonymousStruct;
                type.anon_struct = MALLOC(Properties);
                parse_struct(lexer, type.anon_struct);
                break;
            };
            default:
                fatal("Expected type specification, but got '%.*s", SV_ARG(t.text));
            }
        } break;
        case TK_IDENTIFIER: {
            // interface/enum type
            type.kind = TypeKindType;
            type.name = t.text;
        } break;
        case TK_KEYWORD: {
            type.kind = TypeKindBasic;
            switch (t.code) {
            case TSKeywordLSPAny:
                type.basic_type = BasicTypeAny;
                break;
            case TSKeywordBoolean:
                type.basic_type = BasicTypeBool;
                break;
            case TSKeywordInteger:
                type.basic_type = BasicTypeInt;
                break;
            case TSKeywordDocumentURI:
            case TSKeywordString:
            case TSKeywordURI:
                type.basic_type = BasicTypeString;
                break;
            case TSKeywordNull:
                type.kind = TypeKindConstant;
                type.constant.type = BasicTypeNull;
                break;
            case TSKeywordUInteger:
                type.basic_type = BasicTypeUnsigned;
                break;
            default:
                fatal("Expected type specification, but got '%.*s", SV_ARG(t.text));
            }
        } break;
        case TK_NUMBER: {
            type.kind = TypeKindConstant;
            type.constant.type = BasicTypeInt;
            IntegerParseResult parse_result = sv_parse_i32(t.text);
            assert(parse_result.success);
            type.constant.int_value = parse_result.integer.u32;
        } break;
        case TK_QUOTED_STRING: {
            type.kind = TypeKindConstant;
            type.constant.type = BasicTypeString;
            type.constant.string_value = t.text;
        } break;
        default:
            fatal("Expected type specification, but got '%.*s", SV_ARG(t.text));
        }
        t = lexer_next(lexer);
        if (t.kind == TK_SYMBOL && t.code == '[') {
            lexer_lex(lexer);
            MUST(Token, lexer_expect(lexer, TK_SYMBOL, ']', "Expected ']' to close '["));
            type.array = true;
        }
        if (t.kind != TK_SYMBOL || (t.code != '|' && t.code != ';')) {
            fatal("Expected '|' or ';', got '%.*s'", SV_ARG(t.text));
        }
        lexer_lex(lexer);
        switch (t.code) {
        case '|': {
            if (ret.kind == TypeKindNone) {
                ret.kind = TypeKindAnonymousVariant;
                ret.anon_variant = MALLOC(Variant);
            }
            da_append_Type(&ret.anon_variant->options, type);
        } break;
        case ';': {
            if (ret.kind == TypeKindAnonymousVariant) {
                da_append_Type(&ret.anon_variant->options, type);
            } else {
                ret = type;
            }
            return ret;
        }
        default:
            UNREACHABLE();
        }
    }
}

void parse_struct(Lexer *lexer, Properties *s)
{
    Token token = { 0 };
    for (token = lexer_next(lexer); token.kind != TK_END_OF_FILE && !token_matches(token, TK_SYMBOL, '}'); token = lexer_next(lexer)) {
        if (!token_matches(token, TK_IDENTIFIER, TC_IDENTIFIER)) {
            fatal("Expected property name, got '%.*s'", SV_ARG(token.text));
        }
        Property *prop = da_append_Property(s, (Property) { token.text });
        lexer_lex(lexer);
        if (lexer_next_matches(lexer, TK_SYMBOL, '?')) {
            prop->optional = true;
            lexer_lex(lexer);
        }
        MUST(Token, lexer_expect(lexer, TK_SYMBOL, ':', "Expected ':' after property name"));
        prop->type = parse_type(lexer);
    }
    if (!token_matches(token, TK_SYMBOL, '}')) {
        fatal("Expected '}', got '%.*s'", SV_ARG(token.text));
    }
    lexer_lex(lexer);
}

void parse_interface(Lexer *lexer)
{
    lexer_lex(lexer);
    Token   name = MUST(Token, lexer_expect(lexer, TK_IDENTIFIER, TC_IDENTIFIER, "Expected interface name"));
    TypeDef type_def = { TypeDefKindInterface, name.text };
    Token   extends_maybe = lexer_next(lexer);
    if (token_matches(extends_maybe, TK_KEYWORD, TSKeywordExtends)) {
        lexer_lex(lexer);
        Token comma;
        do {
            Token base_interface = MUST(Token, lexer_expect(lexer, TK_IDENTIFIER, TC_IDENTIFIER, "Expected base interface name"));
            sl_push(&type_def.interface.extends, base_interface.text);
            comma = lexer_lex(lexer);
            assert(comma.kind == TK_SYMBOL);
            assert(comma.code == ',' || comma.code == '{');
        } while (token_matches(comma, TK_SYMBOL, ','));
    } else {
        MUST(Token, lexer_expect(lexer, TK_SYMBOL, '{', "Expected 'extends' or '{"));
    }
    parse_struct(lexer, &type_def.interface.properties);
    da_append_TypeDef(&typedefs, type_def);
    Module *module = da_element_Module(&modules, modules.size - 1);
    sl_push(&module->types, type_def.name);
}

BasicType get_basic_type_for(Type *type)
{
    switch (type->kind) {
    case TypeKindBasic:
        return type->basic_type;
    case TypeKindConstant:
        return type->constant.type;
    case TypeKindAnonymousVariant:
        assert(type->anon_variant->options.size > 0);
        return get_basic_type_for(type->anon_variant->options.elements);
    default:
        UNREACHABLE();
    }
}

void parse_namespace(Lexer *lexer)
{
    lexer_lex(lexer);
    Token name = MUST(Token, lexer_expect(lexer, TK_IDENTIFIER, TC_IDENTIFIER, "Expected namespace name"));
    Namespace namespace = { name.text };
    MUST(Token, lexer_expect(lexer, TK_SYMBOL, '{', "Expected '{"));
    Token token;
    for (token = lexer_next(lexer); token.kind != TK_END_OF_FILE && !token_matches(token, TK_SYMBOL, '}'); token = lexer_next(lexer)) {
        if (token_matches(token, TK_KEYWORD, TSKeywordExport) || token_matches(token, TK_KEYWORD, TSKeywordConst)) {
            lexer_lex(lexer);
            continue;
        }
        if (!token_matches(token, TK_IDENTIFIER, TC_IDENTIFIER)) {
            fatal("Expected value name, got '%.*s'", SV_ARG(token.text));
        }
        NamespaceValue *value = da_append_NamespaceValue(&namespace.values, (NamespaceValue) { token.text });
        lexer_lex(lexer);
        MUST(Token, lexer_expect(lexer, TK_SYMBOL, ':', "Expected ':'"));
        token = lexer_lex(lexer);
        BasicType value_type;
        switch (token.kind) {
        case TK_KEYWORD: {
            switch (token.code) {
            case TSKeywordInteger:
                value_type = BasicTypeInt;
                break;
            case TSKeywordUInteger:
                value_type = BasicTypeUnsigned;
                break;
            case TSKeywordString:
                value_type = BasicTypeString;
                break;
            default:
                fatal("Expected integer number, 'integer', or 'uinteger', got '%.*s'", SV_ARG(token.text));
            }
        } break;
        case TK_NUMBER: {
            value_type = BasicTypeInt;
            IntegerParseResult parse_result = sv_parse_i32(token.text);
            assert(parse_result.success);
            value->int_value = parse_result.integer.i32;
        } break;
        case TK_IDENTIFIER: {
            TypeDef *type = get_typedef(token.text);
            if (type == NULL) {
                fatal("Unexpected identifier '%.*s' in type specification of enumeration value '%.*s::%.*s'",
                    SV_ARG(token.text), SV_ARG(namespace.name), SV_ARG(value->name));
            }
            assert(type->kind == TypeDefKindAlias);
            value_type = get_basic_type_for(&type->alias_for);
            break;
        }
        default:
            fatal("Unexpected enumeration value type specification for enumeration value '%.*s::%.*s': '%.*s'",
                SV_ARG(namespace.name), SV_ARG(value->name), SV_ARG(token.text));
        }

        if (namespace.value_type != BasicTypeNone && value_type != BasicTypeNone && value_type != namespace.value_type) {
            fatal("Value type mismatch in namespace '%.*s'", SV_ARG(namespace.name));
        }

        MUST(Token, lexer_expect(lexer, TK_SYMBOL, '=', "Expected '='"));

        token = lexer_lex(lexer);
        switch (token.kind) {
        case TK_NUMBER: {
            value_type = BasicTypeInt;
            IntegerParseResult parse_result = sv_parse_i32(token.text);
            assert(parse_result.success);
            value->int_value = parse_result.integer.i32;
        } break;
        case TK_QUOTED_STRING: {
            value_type = BasicTypeString;
            value->string_value = token.text;
        } break;
        default:
            fatal("Unexpected value for namespace value '%.*s::%.*s': '%.*s'",
                SV_ARG(namespace.name), SV_ARG(value->name), SV_ARG(token.text));
        }
        if (namespace.value_type != BasicTypeNone && value_type != namespace.value_type) {
            fatal("Value type mismatch in namespace '%.*s'", SV_ARG(namespace.name));
        }
        if (namespace.value_type == BasicTypeNone) {
            namespace.value_type = value_type;
        }
        MUST(Token, lexer_expect(lexer, TK_SYMBOL, ';', "Expected ';'"));
    }
    if (!token_matches(token, TK_SYMBOL, '}')) {
        fatal("Expected '}', got '%.*s'", SV_ARG(token.text));
    }
    lexer_lex(lexer);
    da_append_Namespace(&namespaces, namespace);
}

void parse_typedef(Lexer *lexer)
{
    lexer_lex(lexer);
    bool  is_basic_type = false;
    Token name = lexer_lex(lexer);
    switch (name.kind) {
    case TK_IDENTIFIER:
        break;
    case TK_KEYWORD: {
        switch (name.code) {
        case TSKeywordInteger:
        case TSKeywordUInteger:
        case TSKeywordDocumentURI:
        case TSKeywordDecimal:
        case TSKeywordURI:
            is_basic_type = true;
            break;
        default:
            fatal("Expected type definition name, got '%.*s'", SV_ARG(name.text));
        }
    } break;
    default:
        fatal("Expected type definition name, got '%.*s'", SV_ARG(name.text));
    }
    MUST(Token, lexer_expect(lexer, TK_SYMBOL, '=', "Expected '=' in definition of type '%.*s'", SV_ARG(name.text)));
    TypeDef type_def = { 0 };
    type_def.name = name.text;
    type_def.kind = TypeDefKindAlias;
    type_def.alias_for = parse_type(lexer);

    // StringView type = typedef_to_string(type_def);
    // printf("%.*s\n", SV_ARG(type));
    // sv_free(type);

    if (!is_basic_type) {
        da_append_TypeDef(&typedefs, type_def);
        Module *module = da_element_Module(&modules, modules.size - 1);
        sl_push(&module->types, type_def.name);
    }
}

Module ts_parse(StringView fname)
{
    StringView buffer = MUST(StringView, read_file_by_name(fname));
    Lexer      lexer = lexer_for_language(&ts_language);
    lexer.whitespace_significant = false;
    lexer.include_comments = false;
    lexer_push_source(&lexer, buffer, fname);

    StringView modname = sv_chop_to_delim(&fname, sv_from(".ts"));
    Module     module = { .name = modname };
    da_append_Module(&modules, module);

    for (Token token = lexer_next(&lexer); token.kind != TK_END_OF_FILE; token = lexer_next(&lexer)) {
        if (token.kind == TK_KEYWORD && token.code == TSKeywordInterface) {
            parse_interface(&lexer);
        } else if (token.kind == TK_KEYWORD && token.code == TSKeywordNamespace) {
            parse_namespace(&lexer);
        } else if (token.kind == TK_KEYWORD && token.code == TSKeywordType) {
            parse_typedef(&lexer);
        } else {
            lexer_lex(&lexer);
        }
    }
    return modules.elements[modules.size - 1];
}

int main(int argc, char **argv)
{
    log_init();
    Module module = ts_parse(sv_from(argv[1]));

    TypeDef *t = get_typedef(sv_from(argv[2]));
    assert(t != NULL);
    JSONValue ctx = typedef_serialize(*t);
    json_set(&ctx, "plural", json_string(sv_printf("%.*ss", SV_ARG(t->name))));

    StringView template = MUST(StringView, read_file_by_name(sv_from("interface.h.in")));
    StringView rendered = MUST(StringView, render_template(template, ctx));
    printf("%.*s\n", SV_ARG(rendered));
    return 0;
}
