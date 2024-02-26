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
    TETTNoType = 0,
    TETTEndOfText,
    TETTIdentifier,
    TETTNumber,
    TETTOperator,
    TETTString,
    TETTTrue,
    TETTFalse,
    TETTNull,
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
    TemplateExpressionToken token;
    StringScanner           ss;
    TemplateNode          **current;
} TemplateParserContext;

typedef struct template_render_scope {
    JSONValue                     scope;
    struct template_render_scope *up;
} TemplateRenderScope;

typedef struct {
    union {
        Template template;
        struct {
            StringBuilder sb;
            StringView    text;
            TemplateNode *node;
        };
    };
    StringBuilder        output;
    TemplateRenderScope *scope;
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
    { BTOGreater, 9, ">" },
    { BTOGreaterEquals, 9, ">=" },
    { BTOLess, 9, "<" },
    { BTOLessEquals, 9, "<=" },
    { BTOEquals, 8, "==" },
    { BTONotEquals, 8, "!=" },
    { BTOCount, -1, NULL },
};

static ErrorOrTemplateExpressionToken expression_parser_next(TemplateParserContext *ctx);
static TemplateOperatorMapping        operator_for_token(TemplateParserContext *ctx, TemplateExpressionToken token, bool binary);
static ErrorOrTemplateExpression      parse_primary_expression(TemplateParserContext *ctx);
static ErrorOrTemplateExpression      parse_expression(TemplateParserContext *ctx);
static ErrorOrTemplateExpression      parse_expression_1(TemplateParserContext *ctx, TemplateExpression *lhs, int min_precedence);

DA_IMPL(Macro);
DA_IMPL(Parameter);
DA_IMPL_TYPE(TemplateExpression, TemplateExpression *);

#define IS_IDENTIFIER_START(ch) (isalpha(ch) || ch == '$' || ch == '_')
#define IS_IDENTIFIER_CHAR(ch) (isalpha(ch) || isdigit(ch) || ch == '$' || ch == '_' || ch == '.')

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
    StringView s = { ctx->sb.ptr + ix, ctx->sb.length - ix };
    trace(CAT_TEMPLATE, "Parsed word '%.*s'", SV_ARG(s));
    RETURN(StringRef, ((StringRef) { ix, ctx->sb.length - ix }));
}

ErrorOrTemplateExpressionToken expression_parser_next(TemplateParserContext *ctx)
{
    if (ctx->token.type != TETTNoType) {
        RETURN(TemplateExpressionToken, ctx->token);
    }
    StringScanner *ss = &ctx->ss;
    ss_skip_whitespace(ss);
    ss_reset(ss);
    char                        ch = ss_peek(ss);
    size_t                      ix = ctx->sb.length;
    TemplateExpressionTokenType type;
    TemplateExpressionToken     token = { TETTEndOfText, (StringRef) { 0 } };
    if (IS_IDENTIFIER_START(ch)) {
        type = TETTIdentifier;
        for (ch = ss_peek(ss); IS_IDENTIFIER_CHAR(ch); ch = ss_peek(ss)) {
            sb_append_char(&ctx->sb, ch);
            ss_skip_one(ss);
        }
        StringView s = (StringView) { ctx->sb.ptr + ix, ctx->sb.length - ix };
        if (sv_eq_cstr(s, "true")) {
            type = TETTTrue;
        } else if (sv_eq_cstr(s, "false")) {
            type = TETTFalse;
        } else if (sv_eq_cstr(s, "null")) {
            type = TETTNull;
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
                trace(CAT_TEMPLATE, "Parsed end-of-expression token");
                ctx->token = token;
                RETURN(TemplateExpressionToken, token);
            }
            if (ch == ',') {
                ss_skip_one(ss);
                trace(CAT_TEMPLATE, "Parsed comma end-of-expression marker");
                ctx->token = token;
                RETURN(TemplateExpressionToken, token);
            }
            sb_append_char(&ctx->sb, ch);
            ss_skip_one(ss);
        }
    }
    token = (TemplateExpressionToken) { type, (StringRef) { ix, ctx->sb.length - ix } };
    StringView s = { ctx->sb.ptr + ix, ctx->sb.length - ix };
    trace(CAT_TEMPLATE, "Parsed expression token '%.*s'", SV_ARG(s));
    ctx->token = token;
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
    trace(CAT_TEMPLATE, "parse_expression");
    ctx->token = (TemplateExpressionToken) {0};
    TemplateExpression *primary = TRY(TemplateExpression, parse_primary_expression(ctx));
    if (!primary) {
        trace(CAT_TEMPLATE, "No primary expression");
        RETURN(TemplateExpression, NULL);
    }
    trace(CAT_TEMPLATE, "Primary expression parsed; attempt to parse binary expr");
    return parse_expression_1(ctx, primary, 0);
}

ErrorOrTemplateExpression parse_expression_1(TemplateParserContext *ctx, TemplateExpression *lhs, int min_precedence)
{
    trace(CAT_TEMPLATE, "parse_expression_1");
    TemplateExpressionToken lookahead = TRY_TO(TemplateExpressionToken, TemplateExpression, expression_parser_next(ctx));
    TemplateOperatorMapping op = operator_for_token(ctx, lookahead, true);
    while (/*op.binary && */ op.precedence >= min_precedence) {
        ctx->token = (TemplateExpressionToken) {0};
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
        ctx->token = (TemplateExpressionToken) {0};
        TemplateExpression *var = MALLOC(TemplateExpression);
        var->type = TETVariableReference;
        var->text = token.text;
        RETURN(TemplateExpression, var);
    }
    case TETTNumber: {
        ctx->token = (TemplateExpressionToken) {0};
        TemplateExpression *ret = MALLOC(TemplateExpression);
        ret->type = TETNumber;
        IntegerParseResult parse_result = sv_parse_i64(sv(&ctx->sb, token.text));
        assert(parse_result.success);
        ret->number = parse_result.integer.u64;
        RETURN(TemplateExpression, ret);
    }
    case TETTString: {
        ctx->token = (TemplateExpressionToken) {0};
        TemplateExpression *ret = MALLOC(TemplateExpression);
        ret->type = TETString;
        ret->text = (StringRef) { token.text.index + 1, token.text.length - 2 };
        RETURN(TemplateExpression, ret);
    }
    case TETTTrue:
    case TETTFalse: {
        ctx->token = (TemplateExpressionToken) {0};
        TemplateExpression *ret = MALLOC(TemplateExpression);
        ret->type = TETBoolean;
        ret->boolean = token.type == TETTTrue;
        RETURN(TemplateExpression, ret);
    }
    case TETTNull: {
        ctx->token = (TemplateExpressionToken) {0};
        TemplateExpression *ret = MALLOC(TemplateExpression);
        ret->type = TETNull;
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

TemplateNode * find_macro(Template *template, StringRef name)
{
    StringView n = sv(&template->sb, name);
    for (size_t ix = 0; ix < template->macros.size; ++ix) {
        Macro *macro = da_element_Macro(&template->macros, ix);
        StringView m = sv(&template->sb, macro->key);
        if (sv_eq(m, n)) {
            return macro->value;
        }
    }
    return NULL;
}

ErrorOrInt parse(TemplateParserContext *ctx, StringView terminator);

ErrorOrInt parse_call(TemplateParserContext *ctx)
{
    StringScanner *ss = &ctx->ss;
    StringBuilder *sb = &ctx->sb;

    StringRef name = TRY_TO(StringRef, Int, template_parser_identifier(ctx));
    ss_skip_whitespace(ss);
    TemplateNode *macro = find_macro(&ctx->template, name);
    if (macro == NULL) {
        ERROR(Int, TemplateError, 0, "Undefined macro '%.*s' called", SV_ARG(sv(sb, name)));
    }

    TemplateNode *node = MALLOC(TemplateNode);
    node->kind = TNKMacroCall;
    node->macro_call.macro = name;
    for (size_t ix = 0; ix < macro->macro_def.parameters.size; ++ix) {
        ss_skip_whitespace(ss);
        TemplateExpression *arg = TRY_TO(TemplateExpression, Int, parse_expression(ctx));
        if (arg == NULL) {
            ERROR(Int, TemplateError, 0, "Insufficient number of arguments in call of macro '%.*s'", SV_ARG(sv(sb, name)));
        }
        da_append_TemplateExpression(&node->macro_call.arguments, arg);
    }
    *(ctx->current) = node;
    ctx->current = &node->macro_call.contents;
    TRY(Int, parse(ctx, sv_from("end")));

    ctx->current = &node->next;
    trace(CAT_TEMPLATE, "Created call node");
    RETURN(Int, 0);
}

ErrorOrInt parse_for(TemplateParserContext *ctx)
{
    StringScanner *ss = &ctx->ss;
    StringBuilder *sb = &ctx->sb;
    ss_skip_whitespace(ss);
    StringRef variable = TRY_TO(StringRef, Int, template_parser_identifier(ctx));
    StringRef variable2 = { 0 };
    ss_skip_whitespace(ss);
    if (ss_expect(ss, ',')) {
        ss_skip_whitespace(ss);
        variable2 = TRY_TO(StringRef, Int, template_parser_identifier(ctx));
    }
    TemplateExpression *range = TRY_TO(TemplateExpression, Int, parse_expression(ctx));

    TemplateNode *node = MALLOC(TemplateNode);
    node->kind = TNKForLoop;
    node->for_statement.variable = variable;
    node->for_statement.variable2 = variable2;
    node->for_statement.range = range;

    *(ctx->current) = node;
    ctx->current = &node->for_statement.contents;
    TRY(Int, parse(ctx, sv_from("end")));

    ctx->current = &node->next;
    trace(CAT_TEMPLATE, "Created for node");
    RETURN(Int, 0);
}

ErrorOrInt parse_if(TemplateParserContext *ctx)
{
    StringScanner *ss = &ctx->ss;
    StringBuilder *sb = &ctx->sb;
    ss_skip_whitespace(ss);
    TemplateExpression *condition = TRY_TO(TemplateExpression, Int, parse_expression(ctx));

    TemplateNode *node = MALLOC(TemplateNode);
    node->kind = TNKIfStatement;
    node->if_statement.condition = condition;

    *(ctx->current) = node;
    ctx->current = &node->if_statement.true_branch;
    TRY(Int, parse(ctx, sv_from("else")));
    ctx->current = &node->if_statement.false_branch;
    TRY(Int, parse(ctx, sv_from("end")));

    ctx->current = &node->next;
    trace(CAT_TEMPLATE, "Created if node");
    RETURN(Int, 0);
}

ErrorOrInt parse_macro(TemplateParserContext *ctx)
{
    StringScanner *ss = &ctx->ss;
    TemplateNode *node = MALLOC(TemplateNode);
    node->kind = TNKMacroDef;

    node->macro_def.name = TRY_TO(StringRef, Int, template_parser_identifier(ctx));
    ss_skip_whitespace(ss);
    if (!ss_expect_sv(ss, sv_from("=@"))) {
        while (true) {
            Parameter p = { TRY_TO(StringRef, Int, template_parser_identifier(ctx)), JSON_TYPE_NULL };
            StringView param = sv(&ctx->sb, p.key);
            ss_skip_whitespace(ss);
            if (!ss_expect(ss, ':')) {
                ERROR(Int, TemplateError, 0, "Expected ':' trailing macro parameter '%.*s'", SV_ARG(param));
            }
            StringView type = sv(&ctx->sb, TRY_TO(StringRef, Int, template_parser_identifier(ctx)));
            if (sv_eq_cstr(type, "int")) {
                p.value = JSON_TYPE_INT;
            } else if (sv_eq_cstr(type, "string")) {
                p.value = JSON_TYPE_STRING;
            } else if (sv_eq_cstr(type, "bool")) {
                p.value = JSON_TYPE_BOOLEAN;
            } else if (sv_eq_cstr(type, "object")) {
                p.value = JSON_TYPE_OBJECT;
            } else if (sv_eq_cstr(type, "array")) {
                p.value = JSON_TYPE_ARRAY;
            } else {
                ERROR(Int, TemplateError, 0, "Unknown type '%.*s' for macro parameter '%.*s'", SV_ARG(type), SV_ARG(param));
            }
            da_append_Parameter(&node->macro_def.parameters, p);
            ss_skip_whitespace(ss);
            if (!ss_expect(ss, ',')) {
                break;
            }
        }
        ss_skip_whitespace(ss);
    }
    ss_expect_sv(ss, sv_from("%@"));
    *(ctx->current) = node;
    ctx->current = &node->macro_def.contents;
    TRY(Int, parse(ctx, sv_from("end")));
    ctx->current = &node->next;

    Macro macro = {node->macro_def.name, node};
    da_append_Macro(&ctx->template.macros, macro);

    trace(CAT_TEMPLATE, "Created macro definition");
    RETURN(Int, 0);
}

ErrorOrInt parse_set(TemplateParserContext *ctx)
{
    StringScanner *ss = &ctx->ss;
    ss_skip_whitespace(ss);
    StringRef           variable = TRY_TO(StringRef, Int, template_parser_identifier(ctx));
    TemplateExpression *value = TRY_TO(TemplateExpression, Int, parse_expression(ctx));

    TemplateNode *node = MALLOC(TemplateNode);
    node->kind = TNKSetVariable;
    node->set_statement.variable = variable;
    node->set_statement.value = value;

    ctx->current = &node->next;
    trace(CAT_TEMPLATE, "Created set node");
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
                if (sv_eq_cstr(stmt, "call")) {
                    TRY(Int, parse_call(ctx));
                } else if (sv_eq_cstr(stmt, "for")) {
                    TRY(Int, parse_for(ctx));
                } else if (sv_eq_cstr(stmt, "if")) {
                    TRY(Int, parse_if(ctx));
                } else if (sv_eq_cstr(stmt, "macro")) {
                    TRY(Int, parse_macro(ctx));
                } else if (sv_eq_cstr(stmt, "set")) {
                    TRY(Int, parse_set(ctx));
                } else {
                    ERROR(Int, TemplateError, 0, "Unknown statement '%.*s'", SV_ARG(stmt));
                }
            } break;
            case '#':
                TRY(Int, skip_comment(ctx));
                break;
            default:
                break;
            }
        }
        default: {
            size_t index = sb->length;
            bool first = true;
            while (ss_peek(ss) && (ss_peek(ss) != '@' || ss_peek_with_offset(ss, 1) == '@')) {
                if (ss_expect_sv(ss, sv_from("@@"))) {
                    sb_append_char(sb, '@');
                    first = false;
                    continue;
                }
                int ch = ss_peek(ss);
                if (!first || ch != '\n') {
                    sb_append_char(sb, ss_peek(ss));
                }
                ss_skip_one(ss);
                first = false;
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
        case BTOEquals: {
            bool ret;
            if (lhs.type == JSON_TYPE_INT || rhs.type == JSON_TYPE_INT) {
                ret = json_int_value(lhs) == json_int_value(rhs);
            } else if (lhs.type == JSON_TYPE_STRING || rhs.type == JSON_TYPE_STRING) {
                ret = sv_eq(lhs.string, rhs.string);
            } else if (lhs.type == JSON_TYPE_BOOLEAN || rhs.type == JSON_TYPE_BOOLEAN) {
                ret = lhs.boolean == rhs.boolean;
            } else {
                ERROR(JSONValue, TemplateError, 0, "Can't compare your values yet");
            }
            RETURN(JSONValue, json_bool(ret));
        }
        case BTONotEquals: {
            bool ret;
            if (lhs.type == JSON_TYPE_INT || rhs.type == JSON_TYPE_INT) {
                ret = json_int_value(lhs) != json_int_value(rhs);
            } else if (lhs.type == JSON_TYPE_STRING || rhs.type == JSON_TYPE_STRING) {
                ret = !sv_eq(lhs.string, rhs.string);
            } else if (lhs.type == JSON_TYPE_BOOLEAN || rhs.type == JSON_TYPE_BOOLEAN) {
                ret = lhs.boolean != rhs.boolean;
            } else {
                ERROR(JSONValue, TemplateError, 0, "Can't compare your values yet");
            }
            RETURN(JSONValue, json_bool(ret));
        }
        case BTOGreater: {
            bool ret;
            if (lhs.type == JSON_TYPE_INT || rhs.type == JSON_TYPE_INT) {
                ret = json_int_value(lhs) > json_int_value(rhs);
            } else if (lhs.type == JSON_TYPE_STRING || rhs.type == JSON_TYPE_STRING) {
                ret = sv_cmp(lhs.string, rhs.string) > 0;
            } else {
                ERROR(JSONValue, TemplateError, 0, "Can't compare your values yet");
            }
            RETURN(JSONValue, json_bool(ret));
        }
        case BTOGreaterEquals: {
            bool ret;
            if (lhs.type == JSON_TYPE_INT || rhs.type == JSON_TYPE_INT) {
                ret = json_int_value(lhs) >= json_int_value(rhs);
            } else if (lhs.type == JSON_TYPE_STRING || rhs.type == JSON_TYPE_STRING) {
                ret = sv_cmp(lhs.string, rhs.string) >= 0;
            } else {
                ERROR(JSONValue, TemplateError, 0, "Can't compare your values yet");
            }
            RETURN(JSONValue, json_bool(ret));
        }
        case BTOLess: {
            bool ret;
            if (lhs.type == JSON_TYPE_INT || rhs.type == JSON_TYPE_INT) {
                ret = json_int_value(lhs) < json_int_value(rhs);
            } else if (lhs.type == JSON_TYPE_STRING || rhs.type == JSON_TYPE_STRING) {
                ret = sv_cmp(lhs.string, rhs.string) < 0;
            } else {
                ERROR(JSONValue, TemplateError, 0, "Can't compare your values yet");
            }
            RETURN(JSONValue, json_bool(ret));
        }
        case BTOLessEquals: {
            bool ret;
            if (lhs.type == JSON_TYPE_INT || rhs.type == JSON_TYPE_INT) {
                ret = json_int_value(lhs) <= json_int_value(rhs);
            } else if (lhs.type == JSON_TYPE_STRING || rhs.type == JSON_TYPE_STRING) {
                ret = sv_cmp(lhs.string, rhs.string) <= 0;
            } else {
                ERROR(JSONValue, TemplateError, 0, "Can't compare your values yet");
            }
            RETURN(JSONValue, json_bool(ret));
        }
        default:
            UNREACHABLE();
        }
    }
    case TETBoolean: {
        RETURN(JSONValue, json_bool(expr->boolean));
    }
    case TETNull: {
        RETURN(JSONValue, json_null());
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
        StringView           var_name = sl_pop_front(&as_list);
        TemplateRenderScope *scope = ctx->scope;
        OptionalJSONValue    current_var_maybe = { 0 };
        while (scope) {
            current_var_maybe = json_get_sv(&scope->scope, var_name);
            if (current_var_maybe.has_value) {
                break;
            }
            scope = scope->up;
        }
        if (!current_var_maybe.has_value) {
            ERROR(JSONValue, TemplateError, 0, "Variable '%.*s' not there", SV_ARG(var_name));
        }
        JSONValue current_var = current_var_maybe.value;
        while (!sl_empty(&as_list)) {
            var_name = sl_pop_front(&as_list);
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

ErrorOrInt render_node(TemplateRenderContext *ctx, TemplateNode *node);

ErrorOrInt render_macro(TemplateRenderContext *ctx, TemplateNode *node)
{
    StringView name = sv(&ctx->sb, node->macro_call.macro);
    TemplateNode *macro = find_macro(&ctx->template, node->macro_call.macro);
    assert(macro != NULL);

    JSONNVPairs args = {0};
    TemplateRenderScope call_scope = { json_object(), ctx->scope };
    JSONValue varargs = {0};
    StringView param_name = {0};
    for (size_t ix = 0; ix < macro->macro_def.parameters.size; ++ix) {
        Parameter *param = da_element_Parameter(&macro->macro_def.parameters, ix);
        param_name = sv(&ctx->sb, param->key);
        JSONValue value = TRY_TO(JSONValue, Int, evaluate_expression(ctx, node->macro_call.arguments.elements[ix]));
        if (ix == macro->macro_def.parameters.size - 1 && param->value == JSON_TYPE_ARRAY && value.type != JSON_TYPE_ARRAY) {
            varargs = json_array();
            json_append(&varargs, value);
        } else {
            if (value.type != param->value) {
                ERROR(Int, TemplateError, 0, "Type mismatch for parameter '%.*s' of macro '%.*s'", SV_ARG(param_name) ,SV_ARG(name));
            }
            json_set_sv(&call_scope.scope, param_name, value);
        }
    }
    if (varargs.type == JSON_TYPE_ARRAY) {
        for (size_t ix = macro->macro_def.parameters.size; ix < node->macro_call.arguments.size; ++ix) {
            JSONValue value = TRY_TO(JSONValue, Int, evaluate_expression(ctx, node->macro_call.arguments.elements[ix]));
            json_append(&varargs, value);
        }
        json_set_sv(&call_scope.scope, param_name, varargs);
    }


    ctx->scope = &call_scope;
    TRY(Int, render_node(ctx, macro->macro_def.contents));
    ctx->scope = call_scope.up;
    RETURN(Int, 0);
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
            if (node->for_statement.variable2.length == 0 && value.type != JSON_TYPE_ARRAY) {
                ERROR(Int, TemplateError, 0, "Can only iterate over arrays");
            }
            if (node->for_statement.variable2.length > 0 && value.type != JSON_TYPE_OBJECT) {
                ERROR(Int, TemplateError, 0, "Can only iterate over objects");
            }
            StringView var = sv(&ctx->sb, node->for_statement.variable);
            StringView var2 = sv(&ctx->sb, node->for_statement.variable2);
            for (int ix = 0; ix < json_len(&value); ++ix) {
                TemplateRenderScope loop_scope = { json_object(), ctx->scope };
                ctx->scope = &loop_scope;
                if (var2.length == 0) {
                    JSONValue loop_val = MUST_OPTIONAL(JSONValue, json_at(&value, ix));
                    json_set_sv(&loop_scope.scope, var, loop_val);
                } else {
                    JSONValue loop_vals = MUST_OPTIONAL(JSONValue, json_entry_at(&value, ix));
                    json_set_sv(&loop_scope.scope, var, MUST_OPTIONAL(JSONValue, json_at(&loop_vals, 0)));
                    json_set_sv(&loop_scope.scope, var2, MUST_OPTIONAL(JSONValue, json_at(&loop_vals, 1)));
                }
                TemplateRenderScope inner_scope = { json_object(), &loop_scope };
                ctx->scope = &inner_scope;
                TRY(Int, render_node(ctx, node->for_statement.contents));
                ctx->scope = ctx->scope->up->up;
            }
        } break;
        case TNKIfStatement: {
            trace(CAT_TEMPLATE, "Rendering if statement");
            JSONValue condition = TRY_TO(JSONValue, Int, evaluate_expression(ctx, node->if_statement.condition));
            if (condition.type != JSON_TYPE_BOOLEAN) {
                ERROR(Int, TemplateError, 0, "if condition must be boolean");
            }

            TemplateRenderScope block_scope = { json_object(), ctx->scope };
            ctx->scope = &block_scope;
            TemplateNode *branch = condition.boolean ? node->if_statement.true_branch : node->if_statement.false_branch;
            TRY(Int, render_node(ctx, branch));
            ctx->scope = block_scope.up;
        } break;
        case TNKMacroCall: {
            render_macro(ctx, node);
        } break;
        case TNKMacroDef:
            break;
        case TNKSetVariable: {
            trace(CAT_TEMPLATE, "Rendering for loop");
            JSONValue  value = TRY_TO(JSONValue, Int, evaluate_expression(ctx, node->set_statement.value));
            StringView var = sv(&ctx->sb, node->for_statement.variable);
            json_set_sv(&ctx->scope->scope, var, value);
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
    TemplateRenderScope scope = { json_object(), NULL };
    ctx.scope = &scope;
    json_set(&scope.scope, "$", context);

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
