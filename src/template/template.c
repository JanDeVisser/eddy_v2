/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <io.h>
#include <json.h>
#include <sv.h>
#include <template.h>

TemplateNode *template_find_macro(Template template, StringView name)
{
    for (size_t ix = 0; ix < template.macros.size; ++ix) {
        Macro *macro = da_element_Macro(&template.macros, ix);
        if (sv_eq(macro->key, name)) {
            return macro->value;
        }
    }
    return NULL;
}

ErrorOrStringView render_template(StringView template_text, JSONValue context)
{
    Template template = TRY_TO(Template, StringView, template_parse(template_text));
    if (log_category_on(CAT_TEMPLATE)) {
        StringView ast = json_encode(template_node_serialize(template, template.node));
        printf("AST:\n%.*s\n\n", SV_ARG(ast));
    }
    return template_render(template, context);
}

ErrorOrSize render_template_file(StringView template_filename, JSONValue context, StringView output_filename)
{
    StringView template_text = TRY_TO(StringView, Size, read_file_by_name(template_filename));
    StringView rendered = TRY_TO(StringView, Size, render_template(template_text, context));
    return write_file_by_name(output_filename, rendered);
}
