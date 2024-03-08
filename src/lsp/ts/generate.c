/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <lsp/ts/ts.h>
#include <template/template.h>

void generate_typedef(StringView name)
{
    TypeDef *t = get_typedef(name);
    assert(t != NULL);
    JSONValue ctx = typedef_serialize(*t);

    if (t->kind == TypeDefKindInterface) {
        JSONValue iface = MUST_OPTIONAL(JSONValue, json_get(&ctx, "interface"));
        JSONValue properties = MUST_OPTIONAL(JSONValue, json_get(&iface, "properties"));

        json_set(&ctx, "plural", json_string(sv_printf("%.*ss", SV_ARG(t->name))));

        StringView json = json_encode(ctx);
        printf("%.*s\n", SV_ARG(json));

        StringView name_lower = sv_copy(name);
        for (size_t ix = 0; ix < name_lower.length; ++ix) {
            ((char *) name_lower.ptr)[ix] = tolower(name_lower.ptr[ix]);
        }
        StringView h_file = sv_printf("%.*s.h", SV_ARG(name_lower));
        StringView c_file = sv_printf("%.*s.c", SV_ARG(name_lower));
        MUST(Size, render_template_file(sv_from("interface.h.in"), ctx, h_file));
        MUST(Size, render_template_file(sv_from("interface.c.in"), ctx, c_file));
    }
}
