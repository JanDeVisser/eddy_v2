/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "config.h"
#include <base/io.h>
#include <base/json.h>
#include <base/options.h>
#include <base/sv.h>
#include <lsp/ts/ts.h>
#include <template/template.h>

Keyword ts_keywords[] = {
    { "boolean", TSKeywordBoolean },
    { "const", TSKeywordConst },
    { "decimal", TSKeywordDecimal },
    { "enum", TSKeywordEnum },
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
    { NULL, 0 }
};

DA_IMPL(Type);
DA_IMPL(Property);
DA_IMPL(Interface);
DA_IMPL(EnumerationValue);
DA_IMPL(Enumeration);
DA_IMPL(TypeDef);
DA_IMPL(Module);

TypeDefs typedefs = { 0 };
Modules  modules = { 0 };

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

char const *type_def_kind_name(TypeDefKind kind)
{
    switch (kind) {
    case TypeDefKindInterface:
        return "interface";
    case TypeDefKindAlias:
        return "alias";
    case TypeDefKindEnumeration:
        return "enumeration";
    default:
        UNREACHABLE();
    }
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

int main(int argc, char **argv)
{
    char *ts_module = NULL;
    char *type = NULL;
    for (int ix = 1; ix < argc; ++ix) {
        if (!ts_module) {
            if (!strncmp(argv[ix], "--", 2) && (strlen(argv[ix]) > 2)) {
                StringView  option = sv_from(argv[ix] + 2);
                StringView  value = sv_from("true");
                char const *equals = strchr(argv[ix] + 2, '=');
                if (equals) {
                    option = (StringView) { argv[ix] + 2, equals - argv[ix] - 2 };
                    value = sv_from(equals + 1);
                }
                set_option(option, value);
            } else {
                ts_module = argv[ix];
            }
        } else {
            type = argv[ix];
        }
    }
    set_option(sv_from("eddy-dir"), sv_from(EDDY_DIR));
    log_init();

    printf("Reading %s\n", ts_module);
    Module module = ts_parse(sv_from(ts_module));

    JSONValue mod = json_array();
    for (size_t ix = 0; ix < module.types.size; ++ix) {
        StringView name = module.types.strings[ix];
        TypeDef   *type_def = get_typedef(name);
        assert(type_def != NULL);
        JSONValue typedescr = json_object();
        json_set_string(&typedescr, "name", name);
        json_set_cstr(&typedescr, "kind", type_def_kind_name(type_def->kind));
        json_append(&mod, typedescr);
    }
    StringView json = json_encode(mod);
    StringView json_file = sv_printf("%.*s.json", SV_ARG(module.name));
    MUST(Size, write_file_by_name(json_file, json));

    for (size_t ix = 0; ix < module.types.size; ++ix) {
        if (type == NULL || sv_eq_ignore_case_cstr(module.types.strings[ix], type)) {
            printf("Generating %.*s\n", SV_ARG(module.types.strings[ix]));
            generate_typedef(module.types.strings[ix]);
        }
    }

    return 0;
}
