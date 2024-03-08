/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <io.h>
#include <lsp/ts/ts.h>

static Language ts_language = {
    .name = (StringView) { .ptr = "TypeScript", .length = 10 },
    .directives = NULL,
    .preprocessor_trigger = (Token) { 0 },
    .keywords = ts_keywords,
    .directive_handler = NULL,
};

Type parse_type(Lexer *lexer, StringList *dependencies)
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
                parse_struct(lexer, type.anon_struct, dependencies);
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

void parse_struct(Lexer *lexer, Properties *s, StringList *dependencies)
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
        prop->type = parse_type(lexer, dependencies);
        if (prop->type.kind == TypeKindType) {
            if (!sl_has(dependencies, prop->type.name)) {
                sl_push(dependencies, prop->type.name);
            }
        }
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
    parse_struct(lexer, &type_def.interface.properties, &type_def.dependencies);
    da_append_TypeDef(&typedefs, type_def);
    Module *module = da_element_Module(&modules, modules.size - 1);
    sl_push(&module->types, type_def.name);
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
    type_def.alias_for = parse_type(lexer, &type_def.dependencies);

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
