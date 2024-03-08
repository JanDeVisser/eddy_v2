/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LSP_TS_H
#define LSP_TS_H

#include <json.h>
#include <lexer.h>

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

typedef struct type_def {
    TypeDefKind kind;
    StringView  name;
    StringList  dependencies;
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

extern Keyword    ts_keywords[];
extern Namespaces namespaces;
extern TypeDefs   typedefs;
extern Modules    modules;

extern BasicType   get_basic_type_for(Type *type);
extern TypeDef    *get_typedef(StringView name);
extern char const *basic_type_name(BasicType basic_type);
extern JSONValue   constant_serialize(ConstantType constant);
extern StringView  constant_to_string(ConstantType constant);
extern StringView  property_to_string(Property property);
extern StringView  properties_to_string(Properties properties);
extern StringView  type_to_string(Type type);
extern StringView  interface_to_string(Interface interface);
extern StringView  namespace_to_string(Namespace namespace);
extern StringView  typedef_to_string(TypeDef type_def);
extern JSONValue   type_serialize(Type type);
extern JSONValue   interface_serialize(Interface interface);
extern JSONValue   namespace_serialize(Namespace namespace);
extern JSONValue   typedef_serialize(TypeDef type_def);
extern JSONValue   module_serialize(Module module);
extern Type        parse_type(Lexer *lexer, StringList *dependencies);
extern void        parse_struct(Lexer *lexer, Properties *s, StringList *dependencies);
extern void        parse_interface(Lexer *lexer);
extern void        parse_namespace(Lexer *lexer);
extern void        parse_typedef(Lexer *lexer);
extern Module      ts_parse(StringView fname);
extern void        generate_typedef(StringView name);

#endif /* LSP_TS_H */
