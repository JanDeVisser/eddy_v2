/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "json.h"
#include <ctype.h>
#include <lsp/ts/ts.h>

static void add_properties(JSONValue *properties, Interface interface);

JSONValue constant_serialize(ConstantType constant)
{
    JSONValue ret = json_object();
    json_set_cstr(&ret, "type", basic_type_name(constant.type));
    switch (constant.type) {
    case BasicTypeString:
        json_set_string(&ret, basic_type_name(constant.type), constant.string_value);
        json_set_cstr(&ret, "ctype", "StringView");
        json_set_cstr(&ret, "alias", "StringView");
        break;
    case BasicTypeInt:
        json_set_int(&ret, basic_type_name(constant.type), constant.int_value);
        json_set_cstr(&ret, "ctype", "int");
        json_set_cstr(&ret, "alias", "Int");
        break;
    case BasicTypeUnsigned:
        json_set_int(&ret, basic_type_name(constant.type), constant.int_value);
        json_set_cstr(&ret, "ctype", "unsigned int");
        json_set_cstr(&ret, "alias", "UInt32");
        break;
    case BasicTypeBool:
        json_set(&ret, basic_type_name(constant.type), json_bool(constant.bool_value));
        json_set_cstr(&ret, "ctype", "bool");
        json_set_cstr(&ret, "alias", "Bool");
        break;
    case BasicTypeNull:
        json_set(&ret, basic_type_name(constant.type), json_null());
        json_set_cstr(&ret, "ctype", "Null");
        json_set_cstr(&ret, "alias", "Null");
        break;
    default:
        fatal("Cannot serialize constants with basic type %s", basic_type_name(constant.type));
    }
    return ret;
}

JSONValue basic_type_serialize(Type type)
{
    JSONValue ret = json_object();
    json_set_cstr(&ret, "type", basic_type_name(type.basic_type));
    switch (type.basic_type) {
    case BasicTypeAny:
        json_set_cstr(&ret, "ctype", "JSONValue");
        json_set_cstr(&ret, "alias", "JSONValue");
        break;
    case BasicTypeString:
        json_set_cstr(&ret, "ctype", "StringView");
        json_set_cstr(&ret, "alias", "StringView");
        break;
    case BasicTypeInt:
        json_set_cstr(&ret, "ctype", "int");
        json_set_cstr(&ret, "alias", "Int");
        break;
    case BasicTypeUnsigned:
        json_set_cstr(&ret, "ctype", "unsigned int");
        json_set_cstr(&ret, "alias", "UInt32");
        break;
    case BasicTypeBool:
        json_set_cstr(&ret, "ctype", "bool");
        json_set_cstr(&ret, "alias", "Bool");
        break;
    case BasicTypeNull:
        json_set_cstr(&ret, "ctype", "Null");
        json_set_cstr(&ret, "alias", "Null");
        break;
    default:
        fatal("Cannot serialize objects with basic type %s", basic_type_name(type.basic_type));
    }
    return ret;
}

JSONValue type_serialize(Type type)
{
    JSONValue ret = json_object();
    switch (type.kind) {
    case TypeKindBasic:
        json_set_cstr(&ret, "kind", "basic_type");
        json_set(&ret, "basic_type", basic_type_serialize(type));
        break;
    case TypeKindType:
        json_set_cstr(&ret, "kind", "typeref");
        JSONValue typeref = json_object();
        json_set_string(&typeref, "type", type.name);
        json_set_string(&typeref, "ctype", type.name);
        json_set_string(&typeref, "alias", type.name);
        json_set(&ret, "typeref", typeref);
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

JSONValue enumeration_serialize(Enumeration enumeration)
{
    JSONValue ret = json_object();
    json_set_string(&ret, "name", enumeration.name);
    json_set_cstr(&ret, "type", basic_type_name(enumeration.value_type));
    JSONValue values = json_array();
    for (size_t ix = 0; ix < enumeration.values.size; ++ix) {
        EnumerationValue *value = enumeration.values.elements + ix;
        JSONValue         val = json_object();
        json_set_string(&val, "name", value->name);
        char first = value->name.ptr[0];
        ((char *) value->name.ptr)[0] = toupper(first);
        json_set_string(&val, "capitalized", value->name);
        ((char *) value->name.ptr)[0] = first;
        json_set_cstr(&val, "type", basic_type_name(enumeration.value_type));
        switch (enumeration.value_type) {
        case BasicTypeInt:
        case BasicTypeUnsigned:
            json_set_int(&val, basic_type_name(enumeration.value_type), value->int_value);
            break;
        case BasicTypeString:
            json_set_string(&val, basic_type_name(enumeration.value_type), value->string_value);
            break;
        default:
            UNREACHABLE();
        }
        json_append(&values, val);
    }
    json_set(&ret, "values", values);
    return ret;
}

JSONValue typedef_serialize(TypeDef type_def)
{
    JSONValue ret = json_object();
    json_set_string(&ret, "name", type_def.name);
    JSONValue dependencies = json_array();
    for (size_t ix = 0; ix < type_def.dependencies.size; ++ix) {
        json_append(&dependencies, json_string(type_def.dependencies.strings[ix]));
    }
    json_set(&ret, "dependencies", dependencies);
    switch (type_def.kind) {
    case TypeDefKindAlias: {
        json_set_cstr(&ret, "kind", "alias");
        JSONValue alias_for = type_serialize(type_def.alias_for);
        json_set(&ret, "alias", alias_for);
    } break;
    case TypeDefKindInterface: {
        json_set_cstr(&ret, "kind", "interface");
        JSONValue interface = interface_serialize(type_def.interface);
        json_set(&ret, "interface", interface);
    } break;
    case TypeDefKindEnumeration: {
        json_set_cstr(&ret, "kind", "enumeration");
        JSONValue enumeration = enumeration_serialize(type_def.enumeration);
        json_set(&ret, "enumeration", enumeration);
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
