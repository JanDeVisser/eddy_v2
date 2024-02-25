/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <ctype.h>

#include <io.h>
#include <json.h>
#include <sv.h>
#include <template.h>

typedef enum {
    TNKText,
    TNKExpr,
} TemplateNodeKind;

typedef struct {
    StringRef var_reference;
} Expression;

typedef struct template_node {
    TemplateNodeKind kind;
    union {
        StringRef  text;
        Expression expr;
    };
    struct template_node *next;
} TemplateNode;

ERROR_OR_ALIAS(TemplateNode, TemplateNode *)

typedef struct {
    StringBuilder sb;
    StringView template;
    TemplateNode *node;
} Template;

ERROR_OR(Template);

typedef struct {
    Template template;
    StringScanner  ss;
    TemplateNode **current;
} TemplateParserContext;

ErrorOrInt parse_expression(TemplateParserContext *ctx)
{
    StringScanner *ss = &ctx->ss;
    StringBuilder *sb = &ctx->template.sb;
    ss_skip_one(ss);
    ss_skip_whitespace(ss);

    size_t index = sb->view.length;
    for (int ch = ss_peek(ss); ch && !isspace(ch) && ch != '@'; ch = ss_peek(ss)) {
        if (ch == '\\') {
            ss_skip_one(ss);
            ch = ss_peek(ss);
        }
        sb_append_char(sb, ch);
        ss_skip_one(ss);
    }
    if (!ss_peek(ss)) {
        ERROR(Int, TemplateError, 0, "Expected '=@' to close expression");
    }
    ss_skip_whitespace(ss);
    if (!ss_peek(ss)) {
        ERROR(Int, TemplateError, 0, "Expected '=@' to close expression");
    }
    if (ss_peek(ss) != '=' && ss_peek_with_offset(ss, 1) != '@') {
        ERROR(Int, TemplateError, 0, "Expected '=@' to close expression");
    }
    ss_skip(ss, 2);
    if (!ss_peek(ss)) {
        ERROR(Int, TemplateError, 0, "Expected '=@' to close expression");
    }

    *(ctx->current) = MALLOC(TemplateNode);
    (*ctx->current)->kind = TNKExpr;
    (*ctx->current)->expr.var_reference = (StringRef) { index, sb->view.length - index };
    ctx->current = &(*ctx->current)->next;
    trace(CAT_TEMPLATE, "Created expression node");
    RETURN(Int, 0);
}

ErrorOrInt skip_comment(TemplateParserContext *ctx)
{
    StringScanner *ss = &ctx->ss;
    ss_skip_one(ss);
    ss_skip_whitespace(ss);

    do {
        for (int ch = ss_peek(ss); ch && ch != '#'; ch = ss_peek(ss)) {
            ss_skip_one(ss);
        }
        ss_skip_one(ss);
    } while (ss_peek(ss) && ss_peek(ss) != '@');
    if (!ss_peek(ss)) {
        ERROR(Int, TemplateError, 0, "Unclosed comment");
    }
    ss_skip_one(ss);
    RETURN(Int, 0);
}

ErrorOrInt parse(TemplateParserContext *ctx)
{
    StringScanner *ss = &ctx->ss;
    StringBuilder *sb = &ctx->template.sb;
    while (true) {
        switch (ss_peek(ss)) {
        case '\0': {
            RETURN(Int, 0);
        }
        case '@': {
            ss_skip_one(ss);
            int node_type = ss_peek(ss);
            switch (node_type) {
            case '=':
                TRY(Int, parse_expression(ctx));
                break;
            case '%':
                // Control
                break;
            case '#':
                TRY(Int, skip_comment(ctx));
                break;
            default:
                break;
            }
        }
        default: {
            size_t index = sb->length;
            while (ss_peek(ss) && ss_peek(ss) != '@') {
                if (ss_peek(ss) == '\\') {
                    ss_skip_one(ss);
                }
                sb_append_char(sb, ss_peek(ss));
                ss_skip_one(ss);
            }

            *(ctx->current) = MALLOC(TemplateNode);
            (*ctx->current)->kind = TNKText;
            (*ctx->current)->text = (StringRef) { index, sb->length - index };
            trace(CAT_TEMPLATE, "Created text node");
            ctx->current = &(*ctx->current)->next;
        } break;
        }
    }
}

ErrorOrTemplate parse_template(StringView template)
{
    TemplateParserContext ctx = { 0 };
    ctx.template.template = template;
    ctx.ss = ss_create(template);
    ctx.current = &ctx.template.node;
    TRY_TO(Int, Template, parse(&ctx));
    RETURN(Template, ctx.template);
}

TemplateNode *render_node(Template *template, TemplateNode *node, JSONValue context, StringBuilder *sb)
{
    switch (node->kind) {
    case TNKText: {
        trace(CAT_TEMPLATE, "Rendering text node");
        sb_append_sv(sb, sv(&template->sb, node->text));
        break;
    case TNKExpr: {
        trace(CAT_TEMPLATE, "Rendering expression node");
        sb_append_sv(sb, json_get_string(&context, sv_cstr(sv(&template->sb, node->expr.var_reference)), sv_null()));
        break;
    }
    }
    }
    return node->next;
}

ErrorOrStringView template_render(StringView template_text, JSONValue context)
{
    Template template = TRY_TO(Template, StringView, parse_template(template_text));
    StringBuilder sb = { 0 };
    TemplateNode *node = template.node;
    while (node) {
        node = render_node(&template, node, context, &sb);
    }
    RETURN(StringView, sb.view);
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
    StringView rendered = MUST(StringView, template_render(template, context));
    printf("%.*s", SV_ARG(rendered));
    return 0;
}

#endif /* TEMPLATE_RENDER */
