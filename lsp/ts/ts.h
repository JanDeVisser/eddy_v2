/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LSP_TS_H
#define LSP_TS_H

#include <base/json.h>
#include <base/lexer.h>

typedef enum {
    TSKeywordBoolean,
    TSKeywordConst,
    TSKeywordDecimal,
    TSKeywordEnum,
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
} EnumerationValue;

DA_WITH_NAME(EnumerationValue, EnumerationValues);

typedef struct {
    StringView        name;
    BasicType         value_type;
    EnumerationValues values;
} Enumeration;

typedef struct {
    StringView name;
    BasicType  type;
    union {
        StringView string_value;
        int        int_value;
        bool       bool_value;
    };
} ConstantType;

DA_WITH_NAME(Enumeration, Enumerations);

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
    TypeDefKindEnumeration,
} TypeDefKind;

typedef struct type_def {
    TypeDefKind kind;
    StringView  name;
    StringList  dependencies;
    union {
        Type        alias_for;
        Interface   interface;
        Enumeration enumeration;
    };
} TypeDef;

DA_WITH_NAME(TypeDef, TypeDefs);

typedef struct {
    StringView name;
    StringList types;
} Module;

DA_WITH_NAME(Module, Modules);

extern Keyword  ts_keywords[];
extern TypeDefs typedefs;
extern Modules  modules;

extern BasicType   get_basic_type_for(Type *type);
extern TypeDef    *get_typedef(StringView name);
extern char const *basic_type_name(BasicType basic_type);
extern char const *type_def_kind_name(TypeDefKind kind);
extern JSONValue   constant_serialize(ConstantType constant);
extern StringView  constant_to_string(ConstantType constant);
extern StringView  property_to_string(Property property);
extern StringView  properties_to_string(Properties properties);
extern StringView  type_to_string(Type type);
extern StringView  interface_to_string(Interface interface);
extern StringView  enumeration_to_string(Enumeration enumeration);
extern StringView  typedef_to_string(TypeDef type_def);
extern JSONValue   type_serialize(Type type);
extern JSONValue   interface_serialize(Interface interface);
extern JSONValue   enumeration_serialize(Enumeration enumeration);
extern JSONValue   typedef_serialize(TypeDef type_def);
extern JSONValue   module_serialize(Module module);
extern Type        parse_type(Lexer *lexer);
extern void        parse_struct(Lexer *lexer, Properties *s);
extern void        parse_interface(Lexer *lexer);
extern void        parse_enumeration(Lexer *lexer);
extern void        parse_typedef(Lexer *lexer);
extern Module      ts_parse(StringView fname);
extern void        generate_typedef(StringView name);

#endif /* LSP_TS_H */
