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
    TETTEndOfText = 0,
    TETTIdentifier,
    TETTNumber,
    TETTOperator,
    TETTString,
    TETTTrue,
    TETTFalse,
} TemplateExpressionTokenType;

typedef struct {
    TemplateExpressionTokenType type;
    StringRef                   text;
} TemplateExpressionToken;

ERROR_OR(TemplateExpressionToken);

typedef struct {
    union {
        Template template;
        struct {
            StringBuilder sb;
            StringView    text;
            TemplateNode *node;
        };
    };
    StringScanner  ss;
    TemplateNode **current;
} TemplateParserContext;

typedef struct {
    union {
        Template template;
        struct {
            StringBuilder sb;
            StringView    text;
            TemplateNode *node;
        };
    };
    StringBuilder output;
    JSONValue     variables;
} TemplateRenderContext;

typedef struct {
    BinaryTemplateOperator op;
    int                    precedence;
    char const            *token;
} TemplateOperatorMapping;

/*
 * Precedences according to https://en.cppreference.com/w/c/language/operator_precedence
 */
static TemplateOperatorMapping s_operator_mapping[] = {
    { BTOInvalid, -1, "X" },
    { BTOAdd, 11, "+" },
    { BTOSubtract, 11, "+" },
    { BTOMultiply, 12, "-" },
    { BTODivide, 12, "/" },
    { BTOModulo, 12, "%" },
    { BTOCount, -1, NULL },
};

static ErrorOrTemplateExpressionToken expression_parser_next(TemplateParserContext *ctx);
static TemplateOperatorMapping        operator_for_token(TemplateParserContext *ctx, TemplateExpressionToken token, bool binary);
static ErrorOrTemplateExpression      parse_primary_expression(TemplateParserContext *ctx);
static ErrorOrTemplateExpression      parse_expression(TemplateParserContext *ctx);
static ErrorOrTemplateExpression      parse_expression_1(TemplateParserContext *ctx, TemplateExpression *lhs, int min_precedence);

#define IS_IDENTIFIER_CHAR(ch) (isalpha(ch) || ch == '$' || ch == '_' || ch == '.')

ErrorOrStringRef template_parser_identifier(TemplateParserContext *ctx)
{
    StringScanner *ss = &ctx->ss;
    ss_skip_whitespace(ss);
    char   ch = ss_peek(ss);
    size_t ix = ctx->sb.length;

    if (!isalpha(ch)) {
        ERROR(StringRef, TemplateError, 0, "Expected identifier, got '%c'", ch);
    }

    for (ch = ss_peek(ss); isalpha(ch); ch = ss_peek(ss)) {
        sb_append_char(&ctx->sb, ch);
        ss_skip_one(ss);
    }
    RETURN(StringRef, ((StringRef) { ix, ctx->sb.length - ix }));
}

ErrorOrTemplateExpressionToken expression_parser_next(TemplateParserContext *ctx)
{
    StringScanner *ss = &ctx->ss;
    ss_skip_whitespace(ss);
    char                        ch = ss_peek(ss);
    size_t                      ix = ctx->sb.length;
    TemplateExpressionTokenType type;
    TemplateExpressionToken     token = { TETTEndOfText, (StringRef) { 0 } };
    if (IS_IDENTIFIER_CHAR(ch)) {
        type = TETTIdentifier;
        for (ch = ss_peek(ss); IS_IDENTIFIER_CHAR(ch); ch = ss_peek(ss)) {
            sb_append_char(&ctx->sb, ch);
            ss_skip_one(ss);
        }
    } else if (isdigit(ch)) {
        type = TETTNumber;
        for (ch = ss_peek(ss); isdigit(ch); ch = ss_peek(ss)) {
            sb_append_char(&ctx->sb, ch);
            ss_skip_one(ss);
        }
    } else {
        type = TETTOperator;
        for (ch = ss_peek(ss); ch && !IS_IDENTIFIER_CHAR(ch) && !isspace(ch); ch = ss_peek(ss)) {
            if ((ch == '=' || ch == '%') && ss_peek_with_offset(ss, 1) == '@') {
                ss_skip(ss, 2);
                RETURN(TemplateExpressionToken, token);
            }
            sb_append_char(&ctx->sb, ch);
            ss_skip_one(ss);
        }
    }
    token = (TemplateExpressionToken) { type, (StringRef) { ix, ctx->sb.length - ix } };
    RETURN(TemplateExpressionToken, token);
}

TemplateOperatorMapping operator_for_token(TemplateParserContext *ctx, TemplateExpressionToken token, bool binary)
{
    for (int ix = 0; s_operator_mapping[ix].op != BTOCount; ++ix) {
        if (sv_eq_cstr(sv(&ctx->sb, token.text), s_operator_mapping[ix].token) /*&& s_operator_mapping[ix].binary == binary*/) {
            return s_operator_mapping[ix];
        }
    }
    return s_operator_mapping[0];
}

/*
 * Precedence climbing method (https://en.wikipedia.org/wiki/Operator-precedence_parser):
 *
 * parse_expression()
 *    return parse_expression_1(parse_primary(), 0)
 *
 * parse_expression_1(lhs, min_precedence)
 *    lookahead := peek next token
 *    while lookahead is a binary operator whose precedence is >= min_precedence
 *      *op := lookahead
 *      advance to next token
 *      rhs := parse_primary ()
 *      lookahead := peek next token
 *      while lookahead is a binary operator whose precedence is greater
 *              than op's, or a right-associative operator
 *              whose precedence is equal to op's
 *        rhs := parse_expression_1 (rhs, precedence of op + 1)
 *        lookahead := peek next token
 *      lhs := the result of applying op with operands lhs and rhs
 *    return lhs
 */
ErrorOrTemplateExpression parse_expression(TemplateParserContext *ctx)
{
    TemplateExpression *primary = TRY(TemplateExpression, parse_primary_expression(ctx));
    if (!primary) {
        RETURN(TemplateExpression, NULL);
    }
    return parse_expression_1(ctx, primary, 0);
}

ErrorOrTemplateExpression parse_expression_1(TemplateParserContext *ctx, TemplateExpression *lhs, int min_precedence)
{
    TemplateExpressionToken lookahead = TRY_TO(TemplateExpressionToken, TemplateExpression, expression_parser_next(ctx));
    TemplateOperatorMapping op = operator_for_token(ctx, lookahead, true);
    while (/*op.binary && */ op.precedence >= min_precedence) {
        TemplateExpression *rhs = NULL;
        int                 prec = op.precedence;
        rhs = TRY(TemplateExpression, parse_primary_expression(ctx));
        lookahead = TRY_TO(TemplateExpressionToken, TemplateExpression, expression_parser_next(ctx));
        TemplateOperatorMapping op_1 = operator_for_token(ctx, lookahead, true);
        while (/*op_1.binary && */ op_1.precedence > prec) {
            rhs = TRY(TemplateExpression, parse_expression_1(ctx, rhs, prec + 1));
            if (!rhs) {
                RETURN(TemplateExpression, NULL);
            }
            lookahead = TRY_TO(TemplateExpressionToken, TemplateExpression, expression_parser_next(ctx));
            op_1 = operator_for_token(ctx, lookahead, true);
        }
        TemplateExpression *expr = MALLOC(TemplateExpression);
        expr->type = TETBinaryExpression;
        expr->binary.lhs = lhs;
        expr->binary.rhs = rhs;
        expr->binary.op = op.op;
        lhs = expr;
        lookahead = TRY_TO(TemplateExpressionToken, TemplateExpression, expression_parser_next(ctx));
        op = operator_for_token(ctx, lookahead, true);
    }
    RETURN(TemplateExpression, lhs);
}

ErrorOrTemplateExpression parse_primary_expression(TemplateParserContext *ctx)
{
    TemplateExpressionToken token = TRY_TO(TemplateExpressionToken, TemplateExpression, expression_parser_next(ctx));
    switch (token.type) {
    case TETTIdentifier: {
        TemplateExpression *var = MALLOC(TemplateExpression);
        var->type = TETVariableReference;
        var->text = token.text;
        RETURN(TemplateExpression, var);
    }
    case TETTNumber: {
        TemplateExpression *ret = MALLOC(TemplateExpression);
        ret->type = TETNumber;
        IntegerParseResult parse_result = sv_parse_i64(sv(&ctx->sb, token.text));
        assert(parse_result.success);
        ret->number = parse_result.integer.u64;
        RETURN(TemplateExpression, ret);
    }
    case TETTString: {
        TemplateExpression *ret = MALLOC(TemplateExpression);
        ret->type = TETString;
        ret->text = (StringRef) { token.text.index + 1, token.text.length - 2 };
        RETURN(TemplateExpression, ret);
    }
    case TETTTrue:
    case TETTFalse: {
        TemplateExpression *ret = MALLOC(TemplateExpression);
        ret->type = TETBoolean;
        ret->boolean = token.type == TETTTrue;
        RETURN(TemplateExpression, ret);
    }
    default:
        RETURN(TemplateExpression, NULL);
    }
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

ErrorOrInt parse(TemplateParserContext *ctx, StringView terminator);

ErrorOrInt parse_for(TemplateParserContext *ctx)
{
    StringScanner *ss = &ctx->ss;
    StringBuilder *sb = &ctx->sb;
    ss_skip_whitespace(ss);
    StringRef variable = TRY_TO(StringRef, Int, template_parser_identifier(ctx));
    TemplateExpression *range = TRY_TO(TemplateExpression, Int, parse_expression(ctx));
    // ss_skip_whitespace(ss);
    // if (!ss_expect_sv(ss, sv_from("%@"))) {
    //     ERROR(Int, TemplateError, 0, "Expected %@ to close 'for' statement");
    // }

    TemplateNode *node = MALLOC(TemplateNode);
    node->kind = TNKForLoop;
    node->for_statement.variable = variable;
    node->for_statement.range = range;

    TemplateNode **current = ctx->current;
    ctx->current = &node->for_statement.contents;
    TRY(Int, parse(ctx, sv_from("endfor")));

    *current = node;
    ctx->current = &node->next;
    trace(CAT_TEMPLATE, "Created for node");
    RETURN(Int, 0);
}

ErrorOrInt parse(TemplateParserContext *ctx, StringView terminator)
{
    StringScanner *ss = &ctx->ss;
    StringBuilder *sb = &ctx->sb;
    while (true) {
        switch (ss_peek(ss)) {
        case '\0': {
            if (sv_empty(terminator)) {
                RETURN(Int, 0);
            }
            ERROR(Int, TemplateError, 0, "Expected '%.*' block terminator", SV_ARG(terminator));
        }
        case '@': {
            ss_skip_one(ss);
            int node_type = ss_peek(ss);
            switch (node_type) {
            case '=': {
                ss_skip_one(ss);
                TemplateExpression *expr = TRY_TO(TemplateExpression, Int, parse_expression(ctx));
                *(ctx->current) = MALLOC(TemplateNode);
                (*ctx->current)->kind = TNKExpr;
                (*ctx->current)->expr = expr;
                trace(CAT_TEMPLATE, "Created expression node");
                ctx->current = &(*ctx->current)->next;
            } break;
            case '%': {
                ss_skip_one(ss);
                ss_skip_whitespace(ss);
                StringView stmt = sv(&ctx->sb, TRY_TO(StringRef, Int, template_parser_identifier(ctx)));
                if (!sv_empty(terminator) && sv_eq(stmt, terminator)) {
                    ss_skip_whitespace(ss);
                    if (!ss_expect_sv(ss, sv_from("%@"))) {
                        ERROR(Int, TemplateError, 0, "Expected %%@ to close '%.*s' block terminator", SV_ARG(terminator));
                    }
                    RETURN(Int, 0);
                }
                if (sv_eq_cstr(stmt, "for")) {
                    return parse_for(ctx);
                }
                ERROR(Int, TemplateError, 0, "Unknown statement '%.*s'", stmt);
            }
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

ErrorOrTemplate template_parse(StringView template)
{
    TemplateParserContext ctx = { 0 };
    ctx.template.text = template;
    ctx.ss = ss_create(template);
    ctx.current = &ctx.template.node;
    TRY_TO(Int, Template, parse(&ctx, sv_null()));
    RETURN(Template, ctx.template);
}

ErrorOrJSONValue evaluate_expression(TemplateRenderContext *ctx, TemplateExpression *expr)
{
    switch (expr->type) {
    case TETBinaryExpression: {
        JSONValue lhs = TRY(JSONValue, evaluate_expression(ctx, expr->binary.lhs));
        JSONValue rhs = TRY(JSONValue, evaluate_expression(ctx, expr->binary.rhs));
        switch (expr->binary.op) {
        case BTOAdd: {
            if (lhs.type != JSON_TYPE_INT || rhs.type != JSON_TYPE_INT) {
                ERROR(JSONValue, TemplateError, 0, "Can only add integer values");
            }
            int ret = json_int_value(lhs) + json_int_value(rhs);
            RETURN(JSONValue, json_int(ret));
        }
        case BTOSubtract: {
            if (lhs.type != JSON_TYPE_INT || rhs.type != JSON_TYPE_INT) {
                ERROR(JSONValue, TemplateError, 0, "Can only subtract integer values");
            }
            int ret = json_int_value(lhs) - json_int_value(rhs);
            RETURN(JSONValue, json_int(ret));
        }
        case BTOMultiply: {
            if (lhs.type != JSON_TYPE_INT || rhs.type != JSON_TYPE_INT) {
                ERROR(JSONValue, TemplateError, 0, "Can only multiply integer values");
            }
            int ret = json_int_value(lhs) * json_int_value(rhs);
            RETURN(JSONValue, json_int(ret));
        }
        case BTODivide: {
            if (lhs.type != JSON_TYPE_INT || rhs.type != JSON_TYPE_INT) {
                ERROR(JSONValue, TemplateError, 0, "Can only divide integer values");
            }
            int ret = json_int_value(lhs) / json_int_value(rhs);
            RETURN(JSONValue, json_int(ret));
        }
        case BTOModulo: {
            if (lhs.type != JSON_TYPE_INT || rhs.type != JSON_TYPE_INT) {
                ERROR(JSONValue, TemplateError, 0, "Can only take the modulus of integer values");
            }
            int ret = json_int_value(lhs) % json_int_value(rhs);
            RETURN(JSONValue, json_int(ret));
        }
        default:
            UNREACHABLE();
        }
    }
    case TETBoolean: {
        RETURN(JSONValue, json_bool(expr->boolean));
    }
    case TETNumber: {
        RETURN(JSONValue, json_int(expr->number));
    }
    case TETString: {
        RETURN(JSONValue, json_string(sv(&ctx->sb, expr->text)));
    }
    case TETVariableReference: {
        StringList as_list = sv_split(sv(&ctx->sb, expr->text), sv_from("."));
        assert(as_list.size > 0);
        JSONValue current_var = ctx->variables;
        while (!sl_empty(&as_list)) {
            StringView var_name = sl_pop_front(&as_list);
            current_var = TRY_OPTIONAL(JSONValue, json_get_sv(&current_var, var_name),
                JSONValue, TemplateError, "Variable '%.*s' not there", SV_ARG(var_name));
        }
        RETURN(JSONValue, current_var);
    }
    default:
        UNREACHABLE();
    }
    RETURN(JSONValue, json_null());
}

ErrorOrInt render_node(TemplateRenderContext *ctx, TemplateNode *node)
{
    while (node) {
        switch (node->kind) {
        case TNKText: {
            trace(CAT_TEMPLATE, "Rendering text node");
            sb_append_sv(&ctx->output, sv(&ctx->sb, node->text));
        } break;
        case TNKExpr: {
            trace(CAT_TEMPLATE, "Rendering expression node");
            JSONValue  value = TRY_TO(JSONValue, Int, evaluate_expression(ctx, node->expr));
            StringView sv = json_to_string(value);
            sb_append_sv(&ctx->output, sv);
            sv_free(sv);
        } break;
        case TNKForLoop: {
            trace(CAT_TEMPLATE, "Rendering for loop");
            JSONValue value = TRY_TO(JSONValue, Int, evaluate_expression(ctx, node->for_statement.range));
            if (value.type != JSON_TYPE_ARRAY) {
                ERROR(Int, TemplateError, 0, "Can only iterate over arrays");
            }
            StringView var = sv(&ctx->sb, node->for_statement.variable);
            if (json_has_sv(&ctx->variables, var)) {
                ERROR(Int, TemplateError, 0, "Loop variable '%.*s' shadows existing variable", SV_ARG(var));
            }
            for (int ix = 0; ix < json_len(&value); ++ix) {
                JSONValue loop_val = MUST_OPTIONAL(JSONValue, json_at(&value, ix));
                json_set_sv(&ctx->variables, var, loop_val);
                TRY(Int, render_node(ctx, node->for_statement.contents));
            }
            json_delete_sv(&ctx->variables, var);
        } break;
        }
        node = node->next;
    }
    RETURN(Int, 0);
}

ErrorOrStringView template_render(Template template, JSONValue context)
{
    TemplateRenderContext ctx = { 0 };
    ctx.template = template;
    ctx.variables = json_object();
    json_set(&ctx.variables, "$", context);

    TRY_TO(Int, StringView, render_node(&ctx, template.node));
    RETURN(StringView, ctx.output.view);
}


ErrorOrStringView render_template(StringView template_text, JSONValue context)
{
    Template template = TRY_TO(Template, StringView, template_parse(template_text));
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
    printf("%.*s", SV_ARG(rendered));
    return 0;
}

#endif /* TEMPLATE_RENDER */
