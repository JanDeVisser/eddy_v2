/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <template/template.h>

static ErrorOrTemplateExpression              parse_expression_1(TemplateParserContext *ctx, TemplateExpression *lhs, int min_precedence);
static ErrorOrTemplateExpression              parse_primary_expression(TemplateParserContext *ctx);

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
ErrorOrTemplateExpression template_ctx_parse_expression(TemplateParserContext *ctx)
{
    trace(CAT_TEMPLATE, "template_ctx_parse_expression");
    TemplateExpression *primary = TRY(TemplateExpression, parse_primary_expression(ctx));
    if (primary == NULL) {
        trace(CAT_TEMPLATE, "No primary expression");
        RETURN(TemplateExpression, NULL);
    }
    trace(CAT_TEMPLATE, "Primary expression parsed; attempt to parse binary expr");
    return parse_expression_1(ctx, primary, 0);
}

ErrorOrTemplateExpression parse_expression_1(TemplateParserContext *ctx, TemplateExpression *lhs, int min_precedence)
{
    trace(CAT_TEMPLATE, "parse_expression_1");
    OptionalTemplateOperatorMapping op_maybe = TRY_TO(OptionalTemplateOperatorMapping, TemplateExpression,
        template_lexer_operator(ctx));
    while (op_maybe.has_value && op_maybe.value.binary_precedence >= min_precedence) {
        TemplateOperatorMapping op = op_maybe.value;
        TemplateExpression     *rhs = NULL;
        int                     prec = op.binary_precedence;
        template_lexer_consume(ctx);
        if (op.binary_op == BTOCall) {
            TemplateExpression *expr = MALLOC(TemplateExpression);
            expr->type = TETFunctionCall;
            expr->function_call.function = lhs;
            TemplateExpressionToken t;
            do {
                TemplateExpression *arg = TRY(TemplateExpression, template_ctx_parse_expression(ctx));
                if (arg) {
                    da_append_TemplateExpression(&expr->function_call.arguments, arg);
                }
                t = TRY_TO(TemplateExpressionToken, TemplateExpression, template_lexer_require_one_of(ctx, ",)"));
            } while (t.type != TETTSymbol || t.ch != ')');
            template_lexer_consume(ctx);
            lhs = expr;
            op_maybe = TRY_TO(OptionalTemplateOperatorMapping, TemplateExpression, template_lexer_operator(ctx));
            continue;
        }
        rhs = TRY(TemplateExpression, parse_primary_expression(ctx));
        op_maybe = TRY_TO(OptionalTemplateOperatorMapping, TemplateExpression, template_lexer_operator(ctx));
        while (op_maybe.has_value && op_maybe.value.binary_precedence > prec) {
            template_lexer_consume(ctx);
            rhs = TRY(TemplateExpression, parse_expression_1(ctx, rhs, prec + 1));
            op_maybe = TRY_TO(OptionalTemplateOperatorMapping, TemplateExpression, template_lexer_operator(ctx));
        }
        TemplateExpression *expr = MALLOC(TemplateExpression);
        expr->type = TETBinaryExpression;
        expr->binary.lhs = lhs;
        expr->binary.rhs = rhs;
        expr->binary.op = op.binary_op;
        lhs = expr;
    }
    RETURN(TemplateExpression, lhs);
}

ErrorOrTemplateExpression parse_primary_expression(TemplateParserContext *ctx)
{
    TemplateExpressionToken token = TRY_TO(TemplateExpressionToken, TemplateExpression, template_lexer_next(ctx));
    switch (token.type) {
    case TETTIdentifier: {
        template_lexer_consume(ctx);
        TemplateExpression *var = MALLOC(TemplateExpression);
        var->type = TETIdentifier;
        var->raw_text = token.raw_text;
        RETURN(TemplateExpression, var);
    }
    case TETTNumber: {
        template_lexer_consume(ctx);
        TemplateExpression *ret = MALLOC(TemplateExpression);
        ret->type = TETNumber;
        IntegerParseResult parse_result = sv_parse_i32(token.raw_text);
        assert(parse_result.success);
        ret->number = parse_result.integer.i32;
        RETURN(TemplateExpression, ret);
    }
    case TETTString: {
        template_lexer_consume(ctx);
        TemplateExpression *ret = MALLOC(TemplateExpression);
        ret->type = TETString;
        ret->text = token.text;
        RETURN(TemplateExpression, ret);
    }
    case TETTTrue:
    case TETTFalse: {
        template_lexer_consume(ctx);
        TemplateExpression *ret = MALLOC(TemplateExpression);
        ret->type = TETBoolean;
        ret->boolean = token.type == TETTTrue;
        RETURN(TemplateExpression, ret);
    }
    case TETTNull: {
        template_lexer_consume(ctx);
        TemplateExpression *ret = MALLOC(TemplateExpression);
        ret->type = TETNull;
        RETURN(TemplateExpression, ret);
    }
    case TETTOperator: {
        template_lexer_consume(ctx);
        TemplateOperatorMapping op = MUST_OPTIONAL(TemplateOperatorMapping, template_operator_mapping(token.op));
        assert(op.token == token.op);
        if (op.unary_op == InvalidOperator) {
            ERROR(TemplateExpression, TemplateError, __LINE__, "'%s' cannot be used as a unary operator", op.string);
        }
        TemplateExpression *ret = MALLOC(TemplateExpression);
        ret->type = TETUnaryExpression;
        ret->unary.op = op.unary_op;
        ret->unary.operand = TRY(TemplateExpression, template_ctx_parse_expression(ctx));
        if (op.token == TOTOpenCurly) {
            TRY_TO(Bool, TemplateExpression, template_lexer_require_symbol(ctx, '}'));
        }
        RETURN(TemplateExpression, ret);
    }
    default:
        RETURN(TemplateExpression, NULL);
    }
}

ErrorOrInt close_statement(TemplateParserContext *ctx, TemplateNode *node)
{
    TemplateExpressionToken t = TRY_TO(TemplateExpressionToken, Int, template_lexer_next(ctx));
    switch (t.type) {
    case TETTEndOfStatement: {
        template_lexer_consume(ctx);
        *(ctx->current) = node;
        ctx->current = &node->contents;
        TRY_TO(StringView, Int, template_ctx_parse(ctx, "end"));
    } break;
    case TETTEndOfStatementBlock: {
        template_lexer_consume(ctx);
        *(ctx->current) = node;
        node->contents = MALLOC(TemplateNode);
    } break;
    default: {
        StringView s = template_token_to_string(ctx, t);
        ERROR(Int, TemplateError, __LINE__, "Error parsing close of statement: Got '%.*s'", SV_ARG(s));
    }
    }
    ctx->current = &node->next;
    RETURN(Int, 0);
}

ErrorOrInt parse_call(TemplateParserContext *ctx)
{
    StringView    name = TRY_TO(StringView, Int, template_lexer_require_identifier(ctx));
    TemplateNode *macro = template_find_macro(ctx->template, name);
    if (macro == NULL) {
        ERROR(Int, TemplateError, __LINE__, "Undefined macro '%.*s' called", SV_ARG(name));
    }

    TemplateNode *node = MALLOC(TemplateNode);
    node->kind = TNKMacroCall;
    node->macro_call.macro = name;
    for (size_t ix = 0; ix < macro->macro_def.parameters.size; ++ix) {
        TemplateExpression *arg = TRY_TO(TemplateExpression, Int, template_ctx_parse_expression(ctx));
        if (arg == NULL) {
            ERROR(Int, TemplateError, __LINE__, "Insufficient number of arguments in call of macro '%.*s'", SV_ARG(name));
        }
        da_append_TemplateExpression(&node->macro_call.arguments, arg);
    }
    TRY(Int, close_statement(ctx, node));
    trace(CAT_TEMPLATE, "Created call node");
    RETURN(Int, 0);
}

ErrorOrInt parse_for(TemplateParserContext *ctx)
{
    StringView variable = TRY_TO(StringView, Int, template_lexer_require_identifier(ctx));
    StringView variable2 = { 0 };
    if (TRY_TO(Bool, Int, template_lexer_allow_symbol(ctx, ','))) {
        variable2 = TRY_TO(StringView, Int, template_lexer_require_identifier(ctx));
    }
    TRY_TO(Bool, Int, template_lexer_allow_sv(ctx, sv_from("in")));
    TemplateExpression *range = TRY_TO(TemplateExpression, Int, template_ctx_parse_expression(ctx));

    StringView macro_name = { 0 };
    if (TRY_TO(Bool, Int, template_lexer_allow_sv(ctx, sv_from("do")))) {
        macro_name = TRY_TO(StringView, Int, template_lexer_require_identifier(ctx));
        TemplateNode *macro = template_find_macro(ctx->template, macro_name);
        if (macro == NULL) {
            ERROR(Int, TemplateError, __LINE__, "Undefined macro '%.*s' called", SV_ARG(macro_name));
        }
    }

    TemplateNode *node = MALLOC(TemplateNode);
    node->kind = TNKForLoop;
    node->for_statement.variable = variable;
    node->for_statement.variable2 = variable2;
    node->for_statement.range = range;
    node->for_statement.macro = macro_name;
    close_statement(ctx, node);
    trace(CAT_TEMPLATE, "Created for node");
    RETURN(Int, 0);
}

ErrorOrInt parse_if(TemplateParserContext *ctx)
{
    TemplateExpression *condition = TRY_TO(TemplateExpression, Int, template_ctx_parse_expression(ctx));

    TemplateNode *node = MALLOC(TemplateNode);
    node->kind = TNKIfStatement;
    node->if_statement.condition = condition;

    TemplateExpressionToken t = TRY_TO(TemplateExpressionToken, Int, template_lexer_next(ctx));
    switch (t.type) {
    case TETTEndOfStatement: {
        template_lexer_consume(ctx);
        *(ctx->current) = node;
        ctx->current = &node->if_statement.true_branch;
        StringView terminated_by = TRY_TO(StringView, Int, template_ctx_parse(ctx, "else", "end"));
        if (sv_eq_cstr(terminated_by, "else")) {
            ctx->current = &node->if_statement.false_branch;
            TRY_TO(StringView, Int, template_ctx_parse(ctx, "end"));
        }
    } break;
    case TETTEndOfStatementBlock: {
        ERROR(Int, TemplateError, __LINE__, "'if' statements must have content");
    }
    default: {
        StringView s = template_token_to_string(ctx, t);
        ERROR(Int, TemplateError, __LINE__, "Error parsing close of statement: Got '%.*s'", SV_ARG(s));
    }
    }
    ctx->current = &node->next;
    trace(CAT_TEMPLATE, "Created if node");
    RETURN(Int, 0);
}

ErrorOrInt parse_macro(TemplateParserContext *ctx)
{
    TemplateNode *node = MALLOC(TemplateNode);
    node->kind = TNKMacroDef;

    node->macro_def.name = TRY_TO(StringView, Int, template_lexer_require_identifier(ctx));
    while (true) {
        OptionalStringView param_maybe = TRY_TO(OptionalStringView, Int, template_lexer_allow_identifier(ctx));
        if (!param_maybe.has_value) {
            break;
        }
        Parameter p = { param_maybe.value, JSON_TYPE_NULL };
        template_lexer_require_symbol(ctx, ':');
        StringView type = TRY_TO(StringView, Int, template_lexer_require_identifier(ctx));
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
            ERROR(Int, TemplateError, __LINE__, "Unknown type '%.*s' for macro parameter '%.*s'", SV_ARG(type), SV_ARG(p.key));
        }
        da_append_Parameter(&node->macro_def.parameters, p);
        bool comma = TRY_TO(Bool, Int, template_lexer_allow_symbol(ctx, ','));
        if (!comma) {
            break;
        }
    }

    Macro macro = { node->macro_def.name, node };
    da_append_Macro(&ctx->template.macros, macro);
    close_statement(ctx, node);
    trace(CAT_TEMPLATE, "Created macro definition");
    RETURN(Int, 0);
}

ErrorOrInt parse_set(TemplateParserContext *ctx)
{
    StringView          variable = TRY_TO(StringView, Int, template_lexer_require_identifier(ctx));
    TemplateExpression *value = TRY_TO(TemplateExpression, Int, template_ctx_parse_expression(ctx));

    TemplateNode *node = MALLOC(TemplateNode);
    node->kind = TNKSetVariable;
    node->set_statement.variable = variable;
    node->set_statement.value = value;
    close_statement(ctx, node);

    ctx->current = &node->next;
    trace(CAT_TEMPLATE, "Created set node");
    RETURN(Int, 0);
}

ErrorOrStringView template_ctx_parse_nodes(TemplateParserContext *ctx, ...)
{
    va_list        terminators;
    va_start(terminators, ctx);
    StringList terminator_list = { 0 };
    for (char *t = va_arg(terminators, char *); t != NULL; t = va_arg(terminators, char *)) {
        sl_push(&terminator_list, sv_from(t));
    }
    while (true) {
        TemplateExpressionToken token = TRY_TO(TemplateExpressionToken, StringView, template_lexer_peek(ctx));
        switch (token.type) {
        case TETTComment:
            break;
        case TETTEndOfText: {
            if (sl_empty(&terminator_list)) {
                RETURN(StringView, sv_null());
            }
            StringView terminators_str = sl_join(&terminator_list, sv_from(", "));
            ERROR(StringView, TemplateError, __LINE__, "Expected one of '%.*s' block terminators", SV_ARG(terminators_str));
        }
        case TETTStartOfStatement: {
            template_lexer_consume(ctx);
            StringView stmt = TRY(StringView, template_lexer_require_identifier(ctx));
            if (!sl_empty(&terminator_list) && sl_has(&terminator_list, stmt)) {
                TRY_TO(TemplateExpressionToken, StringView, template_lexer_require_type(ctx, TETTEndOfStatement));
                RETURN(StringView, stmt);
            }
            if (sv_eq_cstr(stmt, "call")) {
                TRY_TO(Int, StringView, parse_call(ctx));
            } else if (sv_eq_cstr(stmt, "for")) {
                TRY_TO(Int, StringView, parse_for(ctx));
            } else if (sv_eq_cstr(stmt, "if")) {
                TRY_TO(Int, StringView, parse_if(ctx));
            } else if (sv_eq_cstr(stmt, "macro")) {
                TRY_TO(Int, StringView, parse_macro(ctx));
            } else if (sv_eq_cstr(stmt, "set")) {
                TRY_TO(Int, StringView, parse_set(ctx));
            } else {
                ERROR(StringView, TemplateError, __LINE__, "Unknown statement '%.*s'", SV_ARG(stmt));
            }
        } break;
        case TETTStartOfExpression: {
            template_lexer_consume(ctx);
            TemplateExpression     *expr = TRY_TO(TemplateExpression, StringView, template_ctx_parse_expression(ctx));
            TRY_TO(TemplateExpressionToken, StringView, template_lexer_require_type(ctx, TETTEndOfExpression));
            TemplateNode *node = MALLOC(TemplateNode);
            node->kind = TNKExpr;
            node->expr = expr;
            trace(CAT_TEMPLATE, "Created expression node");
            *(ctx->current) = node;
            ctx->current = &(*ctx->current)->next;
        } break;
        default: {
            StringBuilder sb = {0};
            while (token.type != TETTStartOfExpression && token.type != TETTStartOfStatement && token.type != TETTEndOfText) {
                switch (token.type) {
                case TETTComment:
                    break;
                case TETTSymbol:
                    sb_append_char(&sb, token.ch);
                    break;
                case TETTString:
                    sb_append_sv(&sb, sv(&ctx->sb, token.text));
                    break;
                case TETTWhitespace:
                    if (!sv_eq_cstr(token.raw_text, "\n") || sb.length > 0)
                        // Fall through!
                default:
                    sb_append_sv(&sb, token.raw_text);
                    break;
                }
                template_lexer_consume(ctx);
                token = TRY_TO(TemplateExpressionToken, StringView, template_lexer_peek(ctx));
            }

            TemplateNode *node = MALLOC(TemplateNode);
            node->kind = TNKText;
            node->text = sb_append_sv(&ctx->sb, sb.view);
            sv_free(sb.view);
            trace(CAT_TEMPLATE, "Created text node");
            *(ctx->current) = node;
            ctx->current = &node->next;
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
    TRY_TO(StringView, Template, template_ctx_parse(&ctx));
    RETURN(Template, ctx.template);
}
