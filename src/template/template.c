/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <io.h>
#include <json.h>
#include <sv.h>
#include <template.h>

TemplateNode *template_find_macro(Template template, StringRef name)
{
    StringView n = sv(&template.sb, name);
    for (size_t ix = 0; ix < template.macros.size; ++ix) {
        Macro     *macro = da_element_Macro(&template.macros, ix);
        StringView m = sv(&template.sb, macro->key);
        if (sv_eq(m, n)) {
            return macro->value;
        }
    }
    return NULL;
}

ErrorOrStringView render_template(StringView template_text, JSONValue context)
{
    Template template = TRY_TO(Template, StringView, template_parse(template_text));
    StringView ast = json_encode(template_node_serialize(template, template.node));
    printf("AST:\n%.*s\n\n",SV_ARG(ast));
    return template_render(template, context);
}

#ifdef TEMPLATE_RENDER

int main(int argc, char **argv)
{
    log_init();
    if (argc != 3) {
        printf("Usage: render <template file> <json file>\n");
        exit(1);
    }
    StringView template = MUST(StringView, read_file_by_name(sv_from(argv[1])));
    StringView json = MUST(StringView, read_file_by_name(sv_from(argv[2])));
    JSONValue  context = MUST(JSONValue, json_decode(json));
    StringView rendered = MUST(StringView, render_template(template, context));
    printf("%.*s\n", SV_ARG(rendered));
    return 0;
}

#endif /* TEMPLATE_RENDER */
