/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <ctype.h>

#include <json.h>
#include <template/template.h>

#define TEMPLATEEXPRESSIONTOKENTYPES(S) \
    S(Unknown)                          \
    S(EndOfExpression)                  \
    S(EndOfNestedExpression)            \
    S(EndOfStatement)                   \
    S(Identifier)                       \
    S(Number)                           \
    S(Operator)                         \
    S(String)                           \
    S(True)                             \
    S(False)                            \
    S(Null)

typedef enum {
#undef S
#define S(T) TETT##T,
    TEMPLATEEXPRESSIONTOKENTYPES(S)
#undef S
} TemplateExpressionTokenType;

#define TEMPLATEOPERATORTOKENS(S)                                    \
    S(Asterisk, "-", BTOMultiply, 12, InvalidOperator, -1)           \
    S(Equals, "==", BTOEquals, 8, InvalidOperator, -1)               \
    S(ExclamationPoint, "!", InvalidOperator, -1, UTOInvert, 14)     \
    S(Greater, ">", BTOGreater, 9, InvalidOperator, -1)              \
    S(GreaterEquals, ">=", BTOGreaterEquals, 9, InvalidOperator, -1) \
    S(Less, "<", BTOLess, 9, InvalidOperator, -1)                    \
    S(LessEquals, "<=", BTOLessEquals, 9, InvalidOperator, -1)       \
    S(Minus, "-", BTOSubtract, 11, UTONegate, 14)                    \
    S(OpenCurly, "{", InvalidOperator, -1, UTODereference, 15)       \
    S(OpenParen, "(", BTOCall, 15, InvalidOperator, -1)              \
    S(Percent, "%", BTOModulo, 12, InvalidOperator, -1)              \
    S(Period, ".", BTOSubscript, 15, InvalidOperator, -1)            \
    S(Plus, "+", BTOAdd, 11, UTOIdentity, 14)                        \
    S(Slash, "/", BTODivide, 12, InvalidOperator, -1)                \
    S(Unequal, "!=", BTONotEquals, 8, InvalidOperator, -1)

typedef enum {
#undef S
#define S(TOKEN, STR, BINOP, BINPREC, UNOP, UNPREC) TOT##TOKEN,
    TEMPLATEOPERATORTOKENS(S)
#undef S
        TOTCount,
} TemplateOperatorToken;

OPTIONAL(TemplateOperatorToken)
ERROR_OR(TemplateOperatorToken)
ERROR_OR(OptionalTemplateOperatorToken)

typedef struct {
    TemplateExpressionTokenType type;
    union {
        StringRef             text;
        TemplateOperatorToken op;
    };
} TemplateExpressionToken;

ERROR_OR(TemplateExpressionToken)

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

typedef struct {
    TemplateOperatorToken token;
    char const           *string;
    TemplateOperator      binary_op;
    int                   binary_precedence;
    TemplateOperator      unary_op;
    int                   unary_precedence;
} TemplateOperatorMapping;

OPTIONAL(TemplateOperatorMapping)
ERROR_OR(TemplateOperatorMapping)
ERROR_OR(OptionalTemplateOperatorMapping)

/*
 * Precedences according to https://en.cppreference.com/w/c/language/operator_precedence
 */
static TemplateOperatorMapping s_operator_mapping[] = {
#undef S
#define S(TOKEN, STR, BINOP, BINPREC, UNOP, UNPREC) { TOT##TOKEN, STR, BINOP, BINPREC, UNOP, UNPREC },
    TEMPLATEOPERATORTOKENS(S) { TOTCount, "X", InvalidOperator, -1, InvalidOperator, -1 },
#undef S
};

static char const                    *TemplateExpressionTokenType_name(TemplateExpressionTokenType type);
static char const                    *TemplateOperatorToken_name(TemplateOperatorToken token);
static ErrorOrTemplateExpressionToken expression_parser_next(TemplateParserContext *ctx, OptionalStringView terminator);
static TemplateOperatorMapping        operator_for_token(TemplateParserContext *ctx, TemplateExpressionToken token);
static ErrorOrTemplateExpression      parse_primary_expression(TemplateParserContext *ctx, OptionalStringView termination);
static ErrorOrTemplateExpression      parse_expression(TemplateParserContext *ctx, OptionalStringView termination);
static ErrorOrTemplateExpression      parse_expression_1(TemplateParserContext *ctx, TemplateExpression *lhs, int min_precedence, OptionalStringView terminator);

DA_IMPL(Macro);
DA_IMPL(Parameter);
DA_IMPL_TYPE(TemplateExpression, TemplateExpression *);

#define IS_IDENTIFIER_START(ch) (isalpha(ch) || ch == '$' || ch == '_')
#define IS_IDENTIFIER_CHAR(ch) (isalpha(ch) || isdigit(ch) || ch == '$' || ch == '_')

char const *TemplateNodeKind_name(TemplateNodeKind kind)
{
    switch (kind) {
#undef KIND
#define KIND(K)  \
    case TNK##K: \
        return #K;
        TEMPLATENODEKINDS(KIND)
#undef KIND
    default:
        UNREACHABLE();
    }
}

char const *TemplateExpressionType_name(TemplateExpressionType type)
{
    switch (type) {
#undef TYPE
#define TYPE(T)  \
    case TET##T: \
        return #T;
        TEMPLATEEXPRESSIONTYPES(TYPE)
#undef KIND
    default:
        UNREACHABLE();
    }
}

char const *TemplateExpressionTokenType_name(TemplateExpressionTokenType type)
{
    switch (type) {
#undef S
#define S(T)      \
    case TETT##T: \
        return #T;
        TEMPLATEEXPRESSIONTYPES(TYPE)
#undef S
    default:
        UNREACHABLE();
    }
}

char const *TemplateOperator_name(TemplateOperator op)
{
    switch (op) {
    case InvalidOperator:
        return "InvalidOperator";
    case BTOCall:
        return "BTOCall";
    case BTOSubscript:
        return "BTOSubscript";
    case BTODereference:
        return "BTODereference";
    case BTOMultiply:
        return "BTOMultiply";
    case BTODivide:
        return "BTODivide";
    case BTOModulo:
        return "BTOModulo";
    case BTOAdd:
        return "BTOAdd";
    case BTOSubtract:
        return "BTOSubtract";
    case BTOGreater:
        return "BTOGreater";
    case BTOGreaterEquals:
        return "BTOGreaterEquals";
    case BTOLess:
        return "BTOLess";
    case BTOLessEquals:
        return "BTOLessEquals";
    case BTOEquals:
        return "BTOEquals";
    case BTONotEquals:
        return "BTONotEquals";
    case UTOIdentity:
        return "UTOIdentity";
    case UTONegate:
        return "UTONegate";
    case UTOInvert:
        return "UTOInvert";
    case UTODereference:
        return "UTODereference";
    default:
        UNREACHABLE();
    }
}

char const *TemplateOperatorToken_name(TemplateOperatorToken token)
{
    switch (token) {
#undef S
#define S(TOKEN, STR, BINOP, BINPREC, UNOP, UNPREC) \
    case TOT##TOKEN:                                \
        return #TOKEN;
        TEMPLATEOPERATORTOKENS(S)
#undef S
    default:
        UNREACHABLE();
    }
}

ErrorOrStringRef template_parser_identifier(TemplateParserContext *ctx)
{
    StringScanner *ss = &ctx->ss;
    ss_skip_whitespace(ss);
    int    ch = ss_peek(ss);
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

void expression_parser_clear(TemplateParserContext *ctx)
{
    ctx->token = (TemplateExpressionToken) { 0 };
}

void expression_parser_consume(TemplateParserContext *ctx)
{
    switch (ctx->token.type) {
    case TETTEndOfNestedExpression:
    case TETTEndOfExpression:
    case TETTEndOfStatement:
        break;
    default:
        expression_parser_clear(ctx);
        break;
    }
}

ErrorOrTemplateExpressionToken expression_parser_next(TemplateParserContext *ctx, OptionalStringView terminator)
{
    if (ctx->token.type != TETTUnknown) {
        RETURN(TemplateExpressionToken, ctx->token);
    }
    StringScanner *ss = &ctx->ss;
    ss_skip_whitespace(ss);
    ss_reset(ss);
    int                         ch = ss_peek(ss);
    size_t                      current_index = ctx->sb.length;
    TemplateExpressionTokenType type;
    TemplateExpressionToken     token = { TETTUnknown, (StringRef) { 0 } };
    if (IS_IDENTIFIER_START(ch)) {
        type = TETTIdentifier;
        for (ch = ss_peek(ss); IS_IDENTIFIER_CHAR(ch); ch = ss_peek(ss)) {
            sb_append_char(&ctx->sb, ch);
            ss_skip_one(ss);
        }
        StringView s = sv_lchop(ctx->sb.view, current_index);
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
        if ((ch == '=' || ch == '%') && ss_peek_with_offset(ss, 1) == '@') {
            ss_skip(ss, 2);
            trace(CAT_TEMPLATE, "Parsed end-of-expression token");
            token.type = TETTEndOfStatement;
            ctx->token = token;
            RETURN(TemplateExpressionToken, token);
        }
        if (ch == ',') {
            ss_skip_one(ss);
            trace(CAT_TEMPLATE, "Parsed comma end-of-expression marker");
            token.type = TETTEndOfExpression;
            ctx->token = token;
            RETURN(TemplateExpressionToken, token);
        }
        if (terminator.has_value && ss_expect_sv(ss, terminator.value)) {
            trace(CAT_TEMPLATE, "Parsed expression termination marker '%.*s'", SV_ARG(terminator.value));
            ss_skip(ss, terminator.value.length);
            token.type = TETTEndOfNestedExpression;
            ctx->token = token;
            RETURN(TemplateExpressionToken, token);
        }

        ss_reset(ss);
        StringView s = sv_lchop(ss->string, ss->mark);
        int        matched = -1;
        StringView matched_op = { 0 };
        for (int ix = 0; ix < TOTCount; ++ix) {
            StringView op = sv_from(s_operator_mapping[ix].string);
            if (sv_startswith(s, op)) {
                if (matched < 0 || sv_length(op) > sv_length(matched_op)) {
                    matched = ix;
                    matched_op = op;
                }
            }
        }
        if (matched >= 0) {
            ss_skip(ss, matched_op.length);
            token = (TemplateExpressionToken) { TETTOperator, (StringRef) { current_index, ctx->sb.length - current_index } };
            trace(CAT_TEMPLATE, "Parsed expression token '%.*s': %s", SV_ARG(matched_op), TemplateOperatorToken_name(matched));
            token.type = TETTOperator;
            token.op = matched;
            ctx->token = token;
            RETURN(TemplateExpressionToken, token);
        }
        ctx->token = token;
        RETURN(TemplateExpressionToken, token);
    }
    token = (TemplateExpressionToken) { type, (StringRef) { current_index, ctx->sb.length - current_index } };
    StringView s = { ctx->sb.ptr + current_index, ctx->sb.length - current_index };
    trace(CAT_TEMPLATE, "Parsed expression token '%.*s'", SV_ARG(s));
    ctx->token = token;
    RETURN(TemplateExpressionToken, token);
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
ErrorOrTemplateExpression parse_expression(TemplateParserContext *ctx, OptionalStringView termination)
{
    trace(CAT_TEMPLATE, "parse_expression");
    TemplateExpression *primary = TRY(TemplateExpression, parse_primary_expression(ctx, termination));
    if (!primary) {
        trace(CAT_TEMPLATE, "No primary expression");
        RETURN(TemplateExpression, NULL);
    }
    trace(CAT_TEMPLATE, "Primary expression parsed; attempt to parse binary expr");
    return parse_expression_1(ctx, primary, 0, termination);
}

ErrorOrOptionalTemplateOperatorMapping next_operator(TemplateParserContext *ctx, OptionalStringView termination)
{
    TemplateExpressionToken lookahead = TRY_TO(TemplateExpressionToken,
        OptionalTemplateOperatorMapping,
        expression_parser_next(ctx, termination));
    switch (lookahead.type) {
    case TETTOperator:
        trace(CAT_TEMPLATE, "next_operator: %s", TemplateOperatorToken_name(lookahead.op));
        RETURN(OptionalTemplateOperatorMapping,
            OptionalTemplateOperatorMapping_create(s_operator_mapping[lookahead.op]));
    case TETTEndOfStatement:
    case TETTEndOfExpression:
    case TETTEndOfNestedExpression:
        trace(CAT_TEMPLATE, "next_operator: empty");
        RETURN(OptionalTemplateOperatorMapping,
            OptionalTemplateOperatorMapping_empty());
    default:
        ERROR(OptionalTemplateOperatorMapping, TemplateError, 0,
            "Expected operator, got '%.*s'", SV_ARG(sv(&ctx->sb, lookahead.text)));
    }
}

ErrorOrTemplateExpression parse_expression_1(TemplateParserContext *ctx, TemplateExpression *lhs, int min_precedence, OptionalStringView terminator)
{
    trace(CAT_TEMPLATE, "parse_expression_1");
    OptionalTemplateOperatorMapping op_maybe = TRY_TO(OptionalTemplateOperatorMapping, TemplateExpression,
        next_operator(ctx, terminator));
    while (op_maybe.has_value && op_maybe.value.binary_precedence >= min_precedence) {
        TemplateOperatorMapping op = op_maybe.value;
        TemplateExpression     *rhs = NULL;
        int                     prec = op.binary_precedence;
        expression_parser_consume(ctx);
        if (op.binary_op == BTOCall) {
            terminator = OptionalStringView_create(sv_from(")"));
            TemplateExpression *expr = MALLOC(TemplateExpression);
            expr->type = TETFunctionCall;
            expr->function_call.function = lhs;
            do {
                TemplateExpression *arg = TRY(TemplateExpression, parse_expression(ctx, terminator));
                if (arg) {
                    da_append_TemplateExpression(&expr->function_call.arguments, arg);
                }
                OptionalTemplateOperatorMapping next_op = TRY_TO(OptionalTemplateOperatorMapping, TemplateExpression,
                    next_operator(ctx, terminator));
                switch (ctx->token.type) {
                case TETTEndOfStatement:
                    ERROR(TemplateExpression, TemplateError, 0, "Expected ')' to terminate function call");
                case TETTEndOfNestedExpression:
                    break;
                default:
                    expression_parser_consume(ctx);
                    break;
                }
            } while (ctx->token.type != TETTEndOfNestedExpression);
            expression_parser_clear(ctx);
            lhs = expr;
            op_maybe = TRY_TO(OptionalTemplateOperatorMapping, TemplateExpression,
                next_operator(ctx, terminator));
            continue;
        }
        rhs = TRY(TemplateExpression, parse_primary_expression(ctx, terminator));
        op_maybe = TRY_TO(OptionalTemplateOperatorMapping, TemplateExpression, next_operator(ctx, terminator));
        while (op_maybe.has_value && op_maybe.value.binary_precedence > prec) {
            rhs = TRY(TemplateExpression, parse_expression_1(ctx, rhs, prec + 1, terminator));
            op_maybe = TRY_TO(OptionalTemplateOperatorMapping, TemplateExpression, next_operator(ctx, terminator));
        }
        expression_parser_consume(ctx);
        TemplateExpression *expr = MALLOC(TemplateExpression);
        expr->type = TETBinaryExpression;
        expr->binary.lhs = lhs;
        expr->binary.rhs = rhs;
        expr->binary.op = op.binary_op;
        lhs = expr;
        op_maybe = TRY_TO(OptionalTemplateOperatorMapping, TemplateExpression, next_operator(ctx, terminator));
    }
    RETURN(TemplateExpression, lhs);
}

ErrorOrTemplateExpression parse_primary_expression(TemplateParserContext *ctx, OptionalStringView termination)
{
    TemplateExpressionToken token = TRY_TO(TemplateExpressionToken, TemplateExpression, expression_parser_next(ctx, termination));
    switch (token.type) {
    case TETTIdentifier: {
        expression_parser_consume(ctx);
        TemplateExpression *var = MALLOC(TemplateExpression);
        var->type = TETIdentifier;
        var->text = token.text;
        RETURN(TemplateExpression, var);
    }
    case TETTNumber: {
        expression_parser_consume(ctx);
        TemplateExpression *ret = MALLOC(TemplateExpression);
        ret->type = TETNumber;
        IntegerParseResult parse_result = sv_parse_i32(sv(&ctx->sb, token.text));
        assert(parse_result.success);
        ret->number = parse_result.integer.i32;
        RETURN(TemplateExpression, ret);
    }
    case TETTString: {
        expression_parser_consume(ctx);
        TemplateExpression *ret = MALLOC(TemplateExpression);
        ret->type = TETString;
        ret->text = (StringRef) { token.text.index + 1, token.text.length - 2 };
        RETURN(TemplateExpression, ret);
    }
    case TETTTrue:
    case TETTFalse: {
        expression_parser_consume(ctx);
        TemplateExpression *ret = MALLOC(TemplateExpression);
        ret->type = TETBoolean;
        ret->boolean = token.type == TETTTrue;
        RETURN(TemplateExpression, ret);
    }
    case TETTNull: {
        expression_parser_consume(ctx);
        TemplateExpression *ret = MALLOC(TemplateExpression);
        ret->type = TETNull;
        RETURN(TemplateExpression, ret);
    }
    case TETTOperator: {
        expression_parser_consume(ctx);
        OptionalStringView      terminator = OptionalStringView_empty();
        TemplateOperatorMapping op = s_operator_mapping[token.op];
        assert(op.token == token.op);
        if (op.unary_op == InvalidOperator) {
            ERROR(TemplateExpression, TemplateError, 0, "'%s' cannot be used as a unary operator", op.string);
        }
        if (op.unary_op == UTODereference) {
            terminator = OptionalStringView_create(sv_from("}"));
        }
        TemplateExpression *ret = MALLOC(TemplateExpression);
        ret->type = TETUnaryExpression;
        ret->unary.op = op.unary_op;
        ret->unary.operand = TRY(TemplateExpression, parse_expression(ctx, terminator));
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

ErrorOrInt parse_call(TemplateParserContext *ctx)
{
    StringScanner *ss = &ctx->ss;
    StringBuilder *sb = &ctx->sb;

    StringRef name = TRY_TO(StringRef, Int, template_parser_identifier(ctx));
    ss_skip_whitespace(ss);
    TemplateNode *macro = template_find_macro(ctx->template, name);
    if (macro == NULL) {
        ERROR(Int, TemplateError, 0, "Undefined macro '%.*s' called", SV_ARG(sv(sb, name)));
    }

    TemplateNode *node = MALLOC(TemplateNode);
    node->kind = TNKMacroCall;
    node->macro_call.macro = name;
    for (size_t ix = 0; ix < macro->macro_def.parameters.size; ++ix) {
        ss_skip_whitespace(ss);
        expression_parser_clear(ctx);
        TemplateExpression *arg = TRY_TO(TemplateExpression, Int, parse_expression(ctx, OptionalStringView_empty()));
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
    expression_parser_clear(ctx);
    TemplateExpression *range = TRY_TO(TemplateExpression, Int, parse_expression(ctx, OptionalStringView_empty()));

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
    expression_parser_clear(ctx);
    TemplateExpression *condition = TRY_TO(TemplateExpression, Int, parse_expression(ctx, OptionalStringView_empty()));

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
    TemplateNode  *node = MALLOC(TemplateNode);
    node->kind = TNKMacroDef;

    node->macro_def.name = TRY_TO(StringRef, Int, template_parser_identifier(ctx));
    ss_skip_whitespace(ss);
    if (!ss_expect_sv(ss, sv_from("=@"))) {
        while (true) {
            Parameter  p = { TRY_TO(StringRef, Int, template_parser_identifier(ctx)), JSON_TYPE_NULL };
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

    Macro macro = { node->macro_def.name, node };
    da_append_Macro(&ctx->template.macros, macro);

    ss_expect_sv(ss, sv_from("%@"));
    *(ctx->current) = node;
    ctx->current = &node->macro_def.contents;
    TRY(Int, parse(ctx, sv_from("end")));
    ctx->current = &node->next;

    trace(CAT_TEMPLATE, "Created macro definition");
    RETURN(Int, 0);
}

ErrorOrInt parse_set(TemplateParserContext *ctx)
{
    StringScanner *ss = &ctx->ss;
    ss_skip_whitespace(ss);
    StringRef variable = TRY_TO(StringRef, Int, template_parser_identifier(ctx));
    expression_parser_clear(ctx);
    TemplateExpression *value = TRY_TO(TemplateExpression, Int, parse_expression(ctx, OptionalStringView_empty()));

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
                expression_parser_clear(ctx);
                TemplateExpression *expr = TRY_TO(TemplateExpression, Int, parse_expression(ctx, OptionalStringView_empty()));
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
            bool   first = true;
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

JSONValue template_expression_serialize(Template tpl, TemplateExpression *expr)
{
    JSONValue   ret = json_object();
    char const *t = TemplateExpressionType_name(expr->type);
    json_set_cstr(&ret, "type", t);
    switch (expr->type) {
    case TETUnaryExpression: {
        JSONValue unary = json_object();
        json_set_cstr(&unary, "operator", TemplateOperator_name(expr->unary.op));
        json_set(&unary, "operand", template_expression_serialize(tpl, expr->unary.operand));
        json_set(&ret, t, unary);
    } break;
    case TETBinaryExpression: {
        JSONValue binary = json_object();
        json_set(&binary, "lhs", template_expression_serialize(tpl, expr->binary.lhs));
        json_set_cstr(&binary, "operator", TemplateOperator_name(expr->binary.op));
        json_set(&binary, "rhs", template_expression_serialize(tpl, expr->binary.rhs));
        json_set(&ret, t, binary);
    } break;
    case TETBoolean:
        json_set(&ret, t, json_bool(expr->boolean));
        break;
    case TETNull:
        json_set(&ret, t, json_null());
        break;
    case TETNumber:
        json_set(&ret, t, json_int(expr->number));
        break;
    case TETString:
    case TETIdentifier:
        json_set(&ret, t, json_string(sv(&tpl.sb, expr->text)));
        break;
    case TETFunctionCall: {
        JSONValue call = json_object();
        json_set(&call, "lhs", template_expression_serialize(tpl, expr->function_call.function));
        JSONValue args = json_array();
        for (size_t ix = 0; ix < expr->function_call.arguments.size; ++ix) {
            json_append(&args, template_expression_serialize(tpl, expr->function_call.arguments.elements[ix]));
        }
        json_set(&call, "arguments", args);
        json_set(&ret, t, call);
    } break;
    default:
        UNREACHABLE();
    }
    return ret;
}

JSONValue template_node_serialize(Template tpl, TemplateNode *node)
{
    JSONValue ret = json_array();
    while (node) {
        JSONValue   n = json_object();
        char const *k = TemplateNodeKind_name(node->kind);
        json_set_cstr(&n, "kind", k);
        switch (node->kind) {
        case TNKText: {
            json_set_string(&n, k, sv(&tpl.sb, node->text));
        } break;
        case TNKExpr: {
            json_set(&n, k, template_expression_serialize(tpl, node->expr));
        } break;
        case TNKForLoop: {
            JSONValue for_loop = json_object();
            json_set_string(&for_loop, "key_variable", sv(&tpl.sb, node->for_statement.variable));
            json_set_string(&for_loop, "value_variable", sv(&tpl.sb, node->for_statement.variable2));
            json_set(&for_loop, "range", template_expression_serialize(tpl, node->for_statement.range));
            json_set(&for_loop, "contents", template_node_serialize(tpl, node->for_statement.contents));
            json_set(&n, k, for_loop);
        } break;
        case TNKIfStatement: {
            JSONValue if_stmt = json_object();
            json_set(&if_stmt, "condition", template_expression_serialize(tpl, node->if_statement.condition));
            json_set(&if_stmt, "on_true", template_node_serialize(tpl, node->if_statement.true_branch));
            json_set(&if_stmt, "on_false", template_node_serialize(tpl, node->if_statement.false_branch));
            json_set(&n, k, if_stmt);
        } break;
        case TNKMacroCall: {
            JSONValue macro_call = json_object();
            json_set(&macro_call, "macro", json_string(sv(&tpl.sb, node->macro_call.macro)));
            JSONValue args = json_array();
            for (size_t ix = 0; node->macro_call.arguments.size; ++ix) {
                json_append(&args, template_expression_serialize(tpl, node->macro_call.arguments.elements[ix]));
            }
            json_set(&macro_call, "arguments", args);
            json_set(&macro_call, "contents", template_node_serialize(tpl, node->macro_call.contents));
            json_set(&n, k, macro_call);
        } break;
        case TNKMacroDef: {
            JSONValue macro_def = json_object();
            json_set(&macro_def, "name", json_string(sv(&tpl.sb, node->macro_def.name)));
            JSONValue params = json_array();
            for (size_t ix = 0; node->macro_def.parameters.size; ++ix) {
                Parameter p = node->macro_def.parameters.elements[ix];
                JSONValue param = json_object();
                json_set_string(&param, "name", sv(&tpl.sb, p.key));
                json_set_cstr(&param, "type", JSONType_name(p.value));
                json_append(&params, param);
            }
            json_set(&macro_def, "parameters", params);
            json_set(&macro_def, "contents", template_node_serialize(tpl, node->macro_def.contents));
            json_set(&n, k, macro_def);
        } break;
        case TNKSetVariable: {
            JSONValue set_var = json_object();
            json_set(&set_var, "variable", json_string(sv(&tpl.sb, node->set_statement.variable)));
            json_set(&set_var, "value", template_expression_serialize(tpl, node->set_statement.value));
            json_set(&n, k, set_var);
        } break;
        default:
            UNREACHABLE();
        }
        json_append(&ret, n);
        node = node->next;
    }
    return ret;
}
