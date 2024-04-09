/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <base/io.h>
#include <lsp/ts/ts.h>
#include <template/template.h>

void generate_typedef(StringView name)
{
    TypeDef *t = get_typedef(name);
    assert(t != NULL);
    JSONValue ctx = typedef_serialize(*t);

    StringView json = json_encode(ctx);
    StringView json_file = sv_printf("%.*s.json", SV_ARG(name));
    MUST(Size, write_file_by_name(json_file, json));
}
