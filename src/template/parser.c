/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "error_or.h"
#include <ctype.h>

#include <base/log.h>
#include <base/sv.h>
#include <template/template.h>

static ErrorOrTemplateExpression parse_expression_1(TemplateParserContext *ctx, TemplateExpression *lhs, int min_precedence);
static ErrorOrTemplateExpression parse_primary_expression(TemplateParserContext *ctx);

DA_IMPL(TplKeyword);

StringView template_expression_to_string(Template tpl, TemplateExpression *expr)
{
    StringBuilder sb = { 0 };
    switch (expr->type) {
    case TETIdentifier:
        sb_append_sv(&sb, expr->raw_text);
        break;
    case TETBinaryExpression: {
        sb_append_char(&sb, '(');
        StringView s = template_expression_to_string(tpl, expr->binary.lhs);
        sb_append_sv(&sb, s);
        sv_free(s);
        sb_printf(&sb, " %s ", TplOperator_name(expr->binary.op));
        s = template_expression_to_string(tpl, expr->binary.rhs);
        sb_append_sv(&sb, s);
        sv_free(s);
        sb_append_char(&sb, ')');
    } break;
    case TETUnaryExpression: {
        sb_printf(&sb, " %s ", TplOperator_name(expr->unary.op));
        StringView s = template_expression_to_string(tpl, expr->unary.operand);
        sb_append_sv(&sb, s);
        sv_free(s);
    } break;
    case TETNumber:
        sb_printf(&sb, "%d", expr->number);
        break;
    case TETString:
        sb_append_sv(&sb, sv(&tpl.sb, expr->text));
        break;
    case TETBoolean:
        sb_printf(&sb, "%s", expr->boolean ? "true" : "false");
        break;
    case TETNull:
        sb_append_cstr(&sb, "(null)");
        break;
    case TETDereference: {
        sb_append_char(&sb, '{');
        StringView s = template_expression_to_string(tpl, expr->dereference);
        sb_append_sv(&sb, s);
        sv_free(s);
        sb_append_char(&sb, '}');
    } break;
    case TETFunctionCall: {
        StringView s = template_expression_to_string(tpl, expr->function_call.function);
        sb_append_sv(&sb, s);
        sb_append_char(&sb, '(');
        for (size_t ix = 0; ix < expr->function_call.arguments.size; ++ix) {
            if (ix > 0) {
                sb_append_cstr(&sb, ", ");
            }
            s = template_expression_to_string(tpl, expr->function_call.arguments.elements[ix]);
            sb_append_sv(&sb, s);
            sv_free(s);
        }
        sb_append_char(&sb, ')');
    } break;
    default:
        UNREACHABLE();
    }
    return sb.view;
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
ErrorOrTemplateExpression template_ctx_parse_expression(TemplateParserContext *ctx)
{
    trace(CAT_TEMPLATE, "template_ctx_parse_expression");
    TemplateExpression *primary = TRY(TemplateExpression, parse_primary_expression(ctx));
    if (primary == NULL) {
        trace(CAT_TEMPLATE, "No primary expression");
        RETURN(TemplateExpression, NULL);
    }
    return parse_expression_1(ctx, primary, 0);
}

ErrorOrTemplateExpression parse_expression_1(TemplateParserContext *ctx, TemplateExpression *lhs, int min_precedence)
{
    trace(CAT_TEMPLATE, "parse_expression_1");
    OptionalTplOperatorMapping op_maybe = TRY_TO(OptionalTplOperatorMapping, TemplateExpression,
        template_lexer_operator(ctx));
    while (op_maybe.has_value && op_maybe.value.binary_precedence >= min_precedence) {
        TplOperatorMapping op = op_maybe.value;
        trace(CAT_TEMPLATE, "parse_expression_1: binary operator %s prec %d >= %d", TplOperator_name(op.binary_op),
            op.binary_precedence, min_precedence);
        TemplateExpression *rhs = NULL;
        int                 prec = op.binary_precedence;
        template_lexer_consume(ctx);
        if (op.binary_op == BTOCall) {
            TemplateExpression *expr = MALLOC(TemplateExpression);
            expr->type = TETFunctionCall;
            expr->function_call.function = lhs;
            TplToken t;
            do {
                TemplateExpression *arg = TRY(TemplateExpression, template_ctx_parse_expression(ctx));
                if (arg) {
                    da_append_TemplateExpression(&expr->function_call.arguments, arg);
                }
                t = TRY_TO(TplToken, TemplateExpression, template_lexer_require_one_of(ctx, ",)"));
            } while (t.type != TTTSymbol || t.ch != ')');
            template_lexer_consume(ctx);
            lhs = expr;
            op_maybe = TRY_TO(OptionalTplOperatorMapping, TemplateExpression, template_lexer_operator(ctx));
            continue;
        }
        rhs = TRY(TemplateExpression, parse_primary_expression(ctx));
        op_maybe = TRY_TO(OptionalTplOperatorMapping, TemplateExpression, template_lexer_operator(ctx));
        while (op_maybe.has_value && op_maybe.value.binary_precedence > prec) {
            trace(CAT_TEMPLATE, "parse_expression_1: binary operator %s prec %d > %d - recursing",
                TplOperator_name(op_maybe.value.binary_op), op_maybe.value.binary_precedence, prec);
            rhs = TRY(TemplateExpression, parse_expression_1(ctx, rhs, prec + 1));
            op_maybe = TRY_TO(OptionalTplOperatorMapping, TemplateExpression, template_lexer_operator(ctx));
        }
        TemplateExpression *expr = MALLOC(TemplateExpression);
        expr->type = TETBinaryExpression;
        expr->binary.lhs = lhs;
        expr->binary.rhs = rhs;
        expr->binary.op = op.binary_op;
        lhs = expr;
    }
    StringView s = template_expression_to_string(ctx->template, lhs);
    trace(CAT_TEMPLATE, "parse_expression_1: returning %.*s", SV_ARG(s));
    sv_free(s);
    RETURN(TemplateExpression, lhs);
}

ErrorOrTemplateExpression parse_primary_expression(TemplateParserContext *ctx)
{
    TplToken            token = TRY_TO(TplToken, TemplateExpression, template_lexer_next(ctx));
    TemplateExpression *ret = NULL;
    switch (token.type) {
    case TTTIdentifier: {
        template_lexer_consume(ctx);
        ret = MALLOC(TemplateExpression);
        ret->type = TETIdentifier;
        ret->raw_text = token.raw_text;
    } break;
    case TTTNumber: {
        template_lexer_consume(ctx);
        IntegerParseResult parse_result = sv_parse_i32(token.raw_text);
        assert(parse_result.success);
        ret = MALLOC(TemplateExpression);
        ret->type = TETNumber;
        ret->number = parse_result.integer.i32;
    } break;
    case TTTString: {
        template_lexer_consume(ctx);
        ret = MALLOC(TemplateExpression);
        ret->type = TETString;
        ret->text = token.text;
    } break;
    case TTTTrue:
    case TTTFalse: {
        template_lexer_consume(ctx);
        ret = MALLOC(TemplateExpression);
        ret->type = TETBoolean;
        ret->boolean = token.type == TTTTrue;
    } break;
    case TTTNull: {
        template_lexer_consume(ctx);
        ret = MALLOC(TemplateExpression);
        ret->type = TETNull;
    } break;
    case TTTOperator: {
        template_lexer_consume(ctx);
        TplOperatorMapping op = MUST_OPTIONAL(TplOperatorMapping, template_operator_mapping(token.op));
        assert(op.token == token.op);
        if (op.unary_op == InvalidOperator) {
            ERROR(TemplateExpression, TemplateError, __LINE__, "'%s' cannot be used as a unary operator", op.string);
        }
        ret = MALLOC(TemplateExpression);
        ret->type = TETUnaryExpression;
        ret->unary.op = op.unary_op;
        ret->unary.operand = TRY(TemplateExpression, template_ctx_parse_expression(ctx));
        if (op.token == TOOpenCurly) {
            TRY_TO(Bool, TemplateExpression, template_lexer_require_symbol(ctx, '}'));
        } else if (op.token == TOOpenParen) {
            TRY_TO(Bool, TemplateExpression, template_lexer_require_symbol(ctx, ')'));
        }
    } break;
    default:
        trace(CAT_TEMPLATE, "parse_primary_expression: not a primary expression");
        RETURN(TemplateExpression, NULL);
    }
    StringView s = template_expression_to_string(ctx->template, ret);
    trace(CAT_TEMPLATE, "parse_primary_expression: returning %.*s", SV_ARG(s));
    sv_free(s);
    RETURN(TemplateExpression, ret);
}

ErrorOrInt close_statement(TemplateParserContext *ctx, TemplateNode *node, bool close_block_allowed)
{
    TplToken t = TRY_TO(TplToken, Int, template_lexer_next(ctx));
    if (t.type != TTTKeyword) {
        StringView s = template_token_to_string(ctx, t);
        ERROR(Int, TemplateError, __LINE__, "Error parsing close of statement: Got '%.*s'", SV_ARG(s));
    }
    template_lexer_consume(ctx);
    switch (t.keyword) {
    case TKWClose: {
        *(ctx->current) = node;
        ctx->current = &node->contents;
        TRY_TO(TplKeyword, Int, template_ctx_parse(ctx, TKWClose));
    } break;
    case TKWCloseBlock: {
        if (close_block_allowed) {
            *(ctx->current) = node;
            node->contents = MALLOC(TemplateNode);
            break;
        }
        // Else fall through
    }
    default: {
        StringView s = template_token_to_string(ctx, t);
        ERROR(Int, TemplateError, __LINE__, "Error parsing close of statement: Got '%.*s'", SV_ARG(s));
    }
    }
    ctx->current = &node->next;
    RETURN(Int, 0);
}

ErrorOrInt parse_call(TemplateParserContext *ctx, TemplateExpression *expr)
{
    StringView name = TRY_TO(StringView, Int, template_lexer_require_identifier(ctx));

    trace(CAT_TEMPLATE, "Parsing call to '%.*s'", SV_ARG(name));
    TemplateNode *node = MALLOC(TemplateNode);
    node->kind = TNKMacroCall;
    node->macro_call.condition = expr;
    node->macro_call.macro = name;
    for (TemplateExpression *arg = TRY_TO(TemplateExpression, Int, template_ctx_parse_expression(ctx));
         arg != NULL; arg = TRY_TO(TemplateExpression, Int, template_ctx_parse_expression(ctx))) {
        da_append_TemplateExpression(&node->macro_call.arguments, arg);
    }
    TRY(Int, close_statement(ctx, node, true));
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

    TemplateExpression *condition = NULL;
    bool                where = TRY_TO(Bool, Int, template_lexer_allow_sv(ctx, sv_from("where")));
    if (where) {
        condition = TRY_TO(TemplateExpression, Int, template_ctx_parse_expression(ctx));
    }

    StringView macro_name = { 0 };
    if (TRY_TO(Bool, Int, template_lexer_allow_sv(ctx, sv_from("do")))) {
        macro_name = TRY_TO(StringView, Int, template_lexer_require_identifier(ctx));
    }

    TemplateNode *node = MALLOC(TemplateNode);
    node->kind = TNKForLoop;
    node->for_statement.variable = variable;
    node->for_statement.variable2 = variable2;
    node->for_statement.range = range;
    node->for_statement.condition = condition;
    node->for_statement.macro = macro_name;
    close_statement(ctx, node, true);
    trace(CAT_TEMPLATE, "Created for node");
    RETURN(Int, 0);
}

ErrorOrInt parse_if(TemplateParserContext *ctx)
{
    TemplateExpression *condition = TRY_TO(TemplateExpression, Int, template_ctx_parse_expression(ctx));

    TemplateNode *node = MALLOC(TemplateNode);
    node->kind = TNKIfStatement;
    node->if_statement.condition = condition;

    TplToken t = TRY_TO(TplToken, Int, template_lexer_next(ctx));
    if (t.type != TTTKeyword) {
        StringView s = template_token_to_string(ctx, t);
        ERROR(Int, TemplateError, __LINE__, "Error parsing close of statement: Got '%.*s'", SV_ARG(s));
    }
    switch (t.keyword) {
    case TKWClose: {
        template_lexer_consume(ctx);
        *(ctx->current) = node;
        ctx->current = &node->if_statement.true_branch;
        TplKeyword terminated_by = TRY_TO(TplKeyword, Int, template_ctx_parse(ctx, TKWClose, TKWElse));
        if (terminated_by == TKWElse) {
            ctx->current = &node->if_statement.false_branch;
            TRY_TO(TplKeyword, Int, template_ctx_parse(ctx, TKWClose));
        }
    } break;
    case TKWCloseBlock: {
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
    close_statement(ctx, node, true);
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
    TplToken t = TRY_TO(TplToken, Int, template_lexer_next(ctx));
    if (t.type != TTTKeyword) {
        StringView s = template_token_to_string(ctx, t);
        ERROR(Int, TemplateError, __LINE__, "Error parsing close of statement: Got '%.*s'", SV_ARG(s));
    }
    template_lexer_consume(ctx);
    switch (t.keyword) {
    case TKWCloseBlock:
    case TKWClose: {
        *(ctx->current) = node;
        node->contents = MALLOC(TemplateNode);
    } break;
    default: {
        StringView s = template_token_to_string(ctx, t);
        ERROR(Int, TemplateError, __LINE__, "Error parsing close of statement: Got '%.*s'", SV_ARG(s));
    }
    }
    ctx->current = &node->next;
    trace(CAT_TEMPLATE, "Created set node");
    RETURN(Int, 0);
}

ErrorOrInt parse_switch(TemplateParserContext *ctx)
{
    TemplateNode *node = MALLOC(TemplateNode);
    node->kind = TNKSwitchStatement;

    TplToken t = TRY_TO(TplToken, Int, template_lexer_next(ctx));
    if (t.type != TTTKeyword) {
        node->switch_statement.expr = TRY_TO(TemplateExpression, Int, template_ctx_parse_expression(ctx));
        t = TRY_TO(TplToken, Int, template_lexer_next(ctx));
    }
    if (t.type != TTTKeyword) {
        StringView s = template_token_to_string(ctx, t);
        ERROR(Int, TemplateError, __LINE__, "Error parsing close of statement: Got '%.*s'", SV_ARG(s));
    }
    switch (t.keyword) {
    case TKWClose: {
        template_lexer_consume(ctx);
        *(ctx->current) = node;
        ctx->current = &node->switch_statement.cases;
        while (true) {
            TplToken   case_else = TRY_TO(TplToken, Int, template_lexer_next(ctx));
            StringView s = template_token_to_string(ctx, t);
            if (case_else.type != TTTKeyword) {
                ERROR(Int, TemplateError, __LINE__, "Error parsing switch statement: Got '%.*s'", SV_ARG(s));
            }
            switch (case_else.keyword) {
            case TKWClose: {
                template_lexer_consume(ctx);
                ctx->current = &node->next;
                trace(CAT_TEMPLATE, "Created switch node");
                RETURN(Int, 0);
            }
            case TKWCase:
            case TKWElse: {
                template_lexer_consume(ctx);
                TemplateExpression *condition = NULL;
                if (case_else.keyword == TKWCase) {
                    condition = TRY_TO(TemplateExpression, Int, template_ctx_parse_expression(ctx));
                    if (node->switch_statement.expr != NULL) {
                        TemplateExpression *case_equals = MALLOC(TemplateExpression);
                        case_equals->type = TETBinaryExpression;
                        case_equals->binary.lhs = node->switch_statement.expr;
                        case_equals->binary.op = BTOEquals;
                        case_equals->binary.rhs = condition;
                        condition = case_equals;
                    }
                }
                TRY_TO(Bool, Int, template_lexer_allow_sv(ctx, sv_from("do")));
                TRY(Int, parse_call(ctx, condition));
            } break;
            default: {
                ERROR(Int, TemplateError, __LINE__, "Error parsing switch statement: Got '%.*s'", SV_ARG(s));
            }
            }
        }
    } break;
    case TKWCloseBlock: {
        ERROR(Int, TemplateError, __LINE__, "'switch' statements must have content");
    }
    default: {
        StringView s = template_token_to_string(ctx, t);
        ERROR(Int, TemplateError, __LINE__, "Error parsing switch statement: Got '%.*s'", SV_ARG(s));
    }
    }
}

ErrorOrInt build_text_node(TemplateParserContext *ctx, StringView text, bool triggered_by_kw)
{
    StringView trimmed = text;
    if (!triggered_by_kw && text.length > 0) {
        if (text.ptr[0] == '\n') {
            size_t ix;
            for (ix = 0; ix < text.length && isspace(text.ptr[ix]); ++ix) {
                ++trimmed.ptr;
            }
            if (ix <= text.length) {
                RETURN(Int, 0);
            }
        }
    }
    TemplateNode *node = MALLOC(TemplateNode);
    node->kind = TNKText;
    node->text = sb_append_sv(&ctx->sb, trimmed);
    sv_free(text);
    trace(CAT_TEMPLATE, "Created text node");
    *(ctx->current) = node;
    ctx->current = &node->next;
    RETURN(Int, 0);
}

ErrorOrInt parse_text(TemplateParserContext *ctx, bool triggered_by_kw)
{
    StringBuilder sb = { 0 };
    bool          end = false;
    for (TplToken token = TRY_TO(TplToken, Int, template_lexer_peek(ctx)); !end && token.type != TTTEndOfText; token = TRY_TO(TplToken, Int, template_lexer_peek(ctx))) {
        switch (token.type) {
        case TTTKeyword:
            if (!triggered_by_kw || (token.keyword == TKWClose || token.keyword == TKWCloseBlock)) {
                return build_text_node(ctx, sb.view, triggered_by_kw);
            }
            sb_append_cstr(&sb, TplKeyword_name(token.keyword));
            break;
        case TTTComment:
            break;
        case TTTSymbol:
            sb_append_char(&sb, token.ch);
            break;
        case TTTString:
            sb_append_sv(&sb, sv(&ctx->sb, token.text));
            break;
        default:
            sb_append_sv(&sb, token.raw_text);
            break;
        }
        template_lexer_consume(ctx);
    }
    return build_text_node(ctx, sb.view, triggered_by_kw);
}

ErrorOrTplKeyword template_ctx_parse_nodes(TemplateParserContext *ctx, ...)
{
    va_list terminators;
    va_start(terminators, ctx);
    TplKeywords terminator_list = { 0 };
    for (TplKeyword kw = va_arg(terminators, TplKeyword); kw != TKWCount; kw = va_arg(terminators, TplKeyword)) {
        da_append_TplKeyword(&terminator_list, kw);
    }
    while (true) {
        TplToken token = TRY_TO(TplToken, TplKeyword, template_lexer_peek(ctx));
        switch (token.type) {
        case TTTComment:
            break;
        case TTTEndOfText: {
            if (terminator_list.size == 0) {
                RETURN(TplKeyword, TKWCount);
            }
            ERROR(TplKeyword, TemplateError, __LINE__, "Expected block terminator");
        }
        case TTTKeyword: {
            template_lexer_consume(ctx);
            switch (token.keyword) {
            case TKWCall:
                TRY_TO(Int, TplKeyword, parse_call(ctx, NULL));
                break;
            case TKWFor:
                TRY_TO(Int, TplKeyword, parse_for(ctx));
                break;
            case TKWIf:
                TRY_TO(Int, TplKeyword, parse_if(ctx));
                break;
            case TKWMacro:
                TRY_TO(Int, TplKeyword, parse_macro(ctx));
                break;
            case TKWSet:
                TRY_TO(Int, TplKeyword, parse_set(ctx));
                break;
            case TKWSwitch:
                TRY_TO(Int, TplKeyword, parse_switch(ctx));
                break;
            case TKWText:
                TRY_TO(Int, TplKeyword, parse_text(ctx, true));
                break;
            case TKWStartExpression: {
                TemplateExpression *expr = TRY_TO(TemplateExpression, TplKeyword, template_ctx_parse_expression(ctx));
                TRY_TO(Bool, TplKeyword, template_lexer_require_keyword(ctx, TKWClose));
                TemplateNode *node = MALLOC(TemplateNode);
                node->kind = TNKExpr;
                node->expr = expr;
                trace(CAT_TEMPLATE, "Created expression node");
                *(ctx->current) = node;
                ctx->current = &(*ctx->current)->next;
            } break;
            default:
                for (size_t ix = 0; ix < terminator_list.size; ++ix) {
                    if (token.keyword == terminator_list.elements[ix]) {
                        RETURN(TplKeyword, token.keyword);
                    }
                }
                ERROR(TplKeyword, TemplateError, 0, "%d:%d: Unhandled keyword '%s'", token.position.line, token.position.column, TplKeyword_name(token.keyword));
            }
        } break;
        default: {
            TRY_TO(Int, TplKeyword, parse_text(ctx, false));
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
    TRY_TO(TplKeyword, Template, template_ctx_parse(&ctx));
    RETURN(Template, ctx.template);
}
