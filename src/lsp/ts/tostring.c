/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <lsp/ts/ts.h>

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
