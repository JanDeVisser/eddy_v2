/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <io.h>
#include <template/template.h>
#include <lsp/ts/ts.h>

Keyword ts_keywords[] = {
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

DA_IMPL(Type);
DA_IMPL(Property);
DA_IMPL(Interface);
DA_IMPL(NamespaceValue);
DA_IMPL(Namespace);
DA_IMPL(TypeDef);
DA_IMPL(Module);

Namespaces namespaces = { 0 };
TypeDefs   typedefs = { 0 };
Modules    modules = { 0 };

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
    log_init();
    printf("Reading %s\n", argv[1]);
    Module module = ts_parse(sv_from(argv[1]));

    for (size_t ix = 0; ix < module.types.size; ++ix) {
        printf("Generating %.*s\n", SV_ARG(module.types.strings[ix]));
        generate_typedef(module.types.strings[ix]);
    }

    return 0;
}
