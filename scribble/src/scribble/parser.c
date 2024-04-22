/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <base/fn.h>
#include <base/fs.h>
#include <base/http.h>
#include <base/lexer.h>
#include <scribble/engine.h>
#include <scribble/model/error.h>
#include <scribble/parser.h>
#include <scribble/type.h>

typedef enum {
    ScribbleDirectiveInclude,
    ScribbleDirectiveMax,
} ScribbleDirective;

typedef enum {
    ScribbleDirectiveStateInit = 0,
    ScribbleDirectiveStateIncludeQuote,
    ScribbleDirectiveStateMacroName,
} ScribbleDirectiveState;

static char const *scribble_directives[ScribbleDirectiveMax + 1] = {
    [ScribbleDirectiveInclude] = "include",
    [ScribbleDirectiveMax] = NULL,
};

// clang-format off
#define SCRIBBLE_KEYWORDS(S)       \
    S(AS, as, 0)                   \
    S(BREAK, break, 1)             \
    S(CONST, const, 2)             \
    S(CONTINUE, continue, 3)       \
    S(ELIF, elif, 4)               \
    S(ELSE, else, 5)               \
    S(ENUM, enum, 6)               \
    S(ERROR, error, 7)             \
    S(FOR, for, 8)                 \
    S(FUNC, func, 9)               \
    S(IF, if, 10)                  \
    S(IMPORT, import, 11)          \
    S(IN, in, 12)                  \
    S(LOOP, loop, 13)              \
    S(MATCH, match, 14)            \
    S(RETURN, return, 15)          \
    S(STRUCT, struct, 16)          \
    S(VAR, var, 17)                \
    S(VARIANT, variant, 18)        \
    S(WHILE, while, 19)            \
    S(TRUE, true, 20)              \
    S(FALSE, false, 21)            \
    S(ASSIGN_BITWISE_AND, &=, 22)  \
    S(ASSIGN_BITWISE_OR, |=, 23)   \
    S(ASSIGN_BITWISE_XOR, ^=, 24)  \
    S(ASSIGN_SHIFT_LEFT, <<=, 25)  \
    S(ASSIGN_SHIFT_RIGHT, >>=, 26) \
    S(BINARY_DECREMENT, -=, 27)    \
    S(BINARY_INCREMENT, +=, 28)    \
    S(ASSIGN_MULTIPLY, *=, 29)     \
    S(ASSIGN_DIVIDE, /=, 30)       \
    S(ASSIGN_MODULO, %=, 31)       \
    S(BIT_SHIFT_LEFT, <<, 32)      \
    S(BIT_SHIFT_RIGHT, >>, 33)     \
    S(EQUALS, ==, 34)              \
    S(GREATER_EQUALS, >=, 35)      \
    S(LESS_EQUALS, <=, 36)         \
    S(LOGICAL_AND, &&, 37)         \
    S(LOGICAL_OR, ||, 38)          \
    S(NOT_EQUALS, !=, 39)          \
    S(RANGE, .., 40)               \
    S(FUNC_BINDING, ->, 41)        \
    S(MACRO_BINDING, =>, 42)       \
    S(UNARY_DECREMENT, --, 43)     \
    S(UNARY_INCREMENT, ++, 44)
// clang-format on

typedef enum : int {
#undef S
#define S(KW, STR, CODE) KW_##KW = CODE,
    SCRIBBLE_KEYWORDS(S)
#undef S
} ScribbleKeyword;

static Keyword scribble_keywords[] = {
#undef S
#define S(KW, STR, CODE) { .keyword = #STR, .code = KW_##KW },
    SCRIBBLE_KEYWORDS(S)
#undef S
        { NULL, 0 },
};

static ScribbleError *parser_context_add_verror(ParserContext *ctx, Token token, char const *msg, va_list args);
static ScribbleError *parser_context_add_error(ParserContext *ctx, Token token, char const *msg, ...);
static void           parser_context_add_vnote(ParserContext *ctx, Token token, char const *msg, va_list args);
static void           parser_context_add_note(ParserContext *ctx, Token token, char const *msg, ...);
static SyntaxNode    *parse_expression(ParserContext *ctx);
static SyntaxNode    *parse_expression_1(ParserContext *ctx, SyntaxNode *lhs, int min_precedence);
static SyntaxNode    *parse_primary_expression(ParserContext *ctx);
static bool           parse_expression_list(ParserContext *ctx, SyntaxNode **dst, char end);
static SyntaxNode    *parse_statement(ParserContext *ctx);
static SyntaxNode    *parse_type(ParserContext *ctx);
static SyntaxNode    *parse_import(ParserContext *ctx);
static SyntaxNode    *parse_module(ParserContext *ctx, StringView buffer, StringView name);
static void           parser_debug_info(ParserContext *ctx, char const *fmt, ...);
static void           parser_debug_node(ParserContext *ctx, SyntaxNode *node);
static int            handle_scribble_directive(Lexer *lexer, int directive);

static Language scribble_language = {
    .name = SV("Scribble"),
    .directives = scribble_directives,
    .preprocessor_trigger = (Token) { .symbol = '$', .kind = TK_SYMBOL },
    .keywords = scribble_keywords,
    .directive_handler = handle_scribble_directive,
};

size_t next_index()
{
    static size_t counter = 0;
    return counter++;
}

SyntaxNode *syntax_node_make(ParserContext *ctx, SyntaxNodeType type, StringView name, Token token)
{
    SyntaxNode *node = (SyntaxNode *) MALLOC(SyntaxNode);
    node->type = type;
    node->name = name;
    node->next = NULL;
    node->index = next_index();
    node->token = token;
    if (ctx && ctx->debug) {
        parser_debug_node(ctx, node);
    }
    return node;
}

ScribbleError *parser_context_add_verror(ParserContext *ctx, Token token, char const *msg, va_list args)
{
    return da_append_ScribbleError(
        &ctx->errors,
        (ScribbleError) {
            .kind = SEK_SYNTAX,
            .token = token,
            .message = sv_vprintf(msg, args),
        });
}

ScribbleError *parser_context_add_error(ParserContext *ctx, Token token, char const *msg, ...)
{
    va_list args;

    va_start(args, msg);
    ScribbleError *ret = parser_context_add_verror(ctx, token, msg, args);
    va_end(args);
    return ret;
}

void parser_context_add_vnote(ParserContext *ctx, Token token, char const *msg, va_list args)
{
    assert(ctx->errors.size > 0);
    ScribbleError *last = da_element_ScribbleError(&ctx->errors, ctx->errors.size - 1);
    da_append_ScribbleError(
        &last->notes,
        (ScribbleError) {
            .kind = last->kind,
            .token = token,
            .message = sv_vprintf(msg, args),
        });
}

void parser_context_add_note(ParserContext *ctx, Token token, char const *msg, ...)
{
    va_list args;

    va_start(args, msg);
    parser_context_add_vnote(ctx, token, msg, args);
    va_end(args);
}

bool parser_context_token_is_error(ParserContext *ctx, ErrorOrToken token_maybe)
{
    if (ErrorOrToken_is_error(token_maybe)) {
        parser_context_add_error(ctx, lexer_lex(ctx->lexer), token_maybe.error.message);
        return true;
    }
    return false;
}

OptionalToken parser_context_expect(ParserContext *ctx, TokenKind kind)
{
    Token token = lexer_next(ctx->lexer);
    if (token.kind != kind) {
        ScribbleError *error = parser_context_add_error(ctx, token,
            "Expected token of kind '%.*s'", SV_ARG(TokenKind_name(kind)));
        RETURN_EMPTY(Token);
    }
    lexer_lex(ctx->lexer);
    RETURN_VALUE(Token, token);
}

bool parser_context_accept(ParserContext *ctx, TokenKind kind)
{
    Token token = lexer_next(ctx->lexer);
    return token.kind == kind;
}

bool parser_context_accept_and_discard(ParserContext *ctx, TokenKind kind)
{
    Token token = lexer_next(ctx->lexer);
    if (token.kind == kind) {
        lexer_lex(ctx->lexer);
        return true;
    }
    return false;
}

bool parser_context_expect_keyword(ParserContext *ctx, ScribbleKeyword kw)
{
    Token token = lexer_next(ctx->lexer);
    if (!token_matches_keyword(token, kw)) {
        ScribbleError *error = parser_context_add_error(ctx, token,
            "Expected keyword '%.*s'", SV_ARG(lexer_keyword(ctx->lexer, kw)));
        return false;
    }
    lexer_lex(ctx->lexer);
    return true;
}

bool parser_context_accept_keyword(ParserContext *ctx, ScribbleKeyword kw)
{
    Token token = lexer_next(ctx->lexer);
    return token_matches_keyword(token, kw);
}

bool parser_context_accept_and_discard_keyword(ParserContext *ctx, ScribbleKeyword kw)
{
    Token token = lexer_next(ctx->lexer);
    if (token_matches_keyword(token, kw)) {
        lexer_lex(ctx->lexer);
        return true;
    }
    return false;
}

bool parser_context_expect_symbol(ParserContext *ctx, int symbol)
{
    Token token = lexer_next(ctx->lexer);
    if (!token_matches_symbol(token, symbol)) {
        ScribbleError *error = parser_context_add_error(ctx, token,
            "Expected '%c'", symbol);
        return false;
    }
    lexer_lex(ctx->lexer);
    return true;
}

bool parser_context_accept_symbol(ParserContext *ctx, int symbol)
{
    Token token = lexer_next(ctx->lexer);
    return token_matches_symbol(token, symbol);
}

bool parser_context_accept_and_discard_symbol(ParserContext *ctx, int symbol)
{
    Token token = lexer_next(ctx->lexer);
    if (token_matches_symbol(token, symbol)) {
        lexer_lex(ctx->lexer);
        return true;
    }
    return false;
}

OptionalToken parser_context_expect_identifier(ParserContext *ctx)
{
    Token token = lexer_next(ctx->lexer);
    if (!token_matches_kind(token, TK_IDENTIFIER)) {
        ScribbleError *error = parser_context_add_error(ctx, token,
            "Expected identifier");
        RETURN_EMPTY(Token);
    }
    lexer_lex(ctx->lexer);
    RETURN_VALUE(Token, token);
}

bool parser_context_accept_identifier(ParserContext *ctx)
{
    Token token = lexer_next(ctx->lexer);
    return token_matches_kind(token, TK_IDENTIFIER);
}

bool parser_context_accept_and_discard_identifier(ParserContext *ctx)
{
    Token token = lexer_next(ctx->lexer);
    if (token_matches_kind(token, TK_IDENTIFIER)) {
        lexer_lex(ctx->lexer);
        return true;
    }
    return false;
}

OptionalToken parser_context_expect_quoted_string(ParserContext *ctx, QuoteType quote)
{
    Token token = lexer_next(ctx->lexer);
    if (token.kind != TK_QUOTED_STRING || token.quoted_string.quote_type != quote) {
        ScribbleError *error = parser_context_add_error(ctx, token,
            "Expected quoted string");
        RETURN_EMPTY(Token);
    }
    lexer_lex(ctx->lexer);
    RETURN_VALUE(Token, token);
}

bool parser_context_accept_quoted_string(ParserContext *ctx, QuoteType quote)
{
    Token token = lexer_next(ctx->lexer);
    return token.kind == TK_QUOTED_STRING && token.quoted_string.quote_type == quote;
}

bool parser_context_accept_and_discard_quoted_string(ParserContext *ctx, QuoteType quote)
{
    Token token = lexer_next(ctx->lexer);
    if (token.kind != TK_QUOTED_STRING || token.quoted_string.quote_type != quote) {
        return false;
    }
    lexer_lex(ctx->lexer);
    return true;
}

#define EXPECT_SYMBOL_OR(ctx, symbol, ret)                    \
    do {                                                      \
        if (!parser_context_expect_symbol((ctx), (symbol))) { \
            return (ret);                                     \
        }                                                     \
    } while (0)

#define EXPECT_SYMBOL(ctx, symbol) EXPECT_SYMBOL_OR(ctx, symbol, NULL)

#define EXPECT_KEYWORD_OR(ctx, kw, ret)                    \
    do {                                                   \
        if (!parser_context_expect_keyword((ctx), (kw))) { \
            return (ret);                                  \
        }                                                  \
    } while (0)

#define EXPECT_KEYWORD(ctx, kw) EXPECT_KEYWORD_OR(ctx, kw, NULL)

#define EXPECT_IDENTIFIER_OR(ctx, ret)                              \
    ({                                                              \
        OptionalToken _t = parser_context_expect_identifier((ctx)); \
        if (!_t.has_value) {                                        \
            return (ret);                                           \
        }                                                           \
        _t.value;                                                   \
    })

#define EXPECT_IDENTIFIER(ctx) EXPECT_IDENTIFIER_OR(ctx, NULL)

#define ACCEPT_SYMBOL_OR(ctx, symbol, ret)                                \
    do {                                                                  \
        if (!parser_context_accept_and_discard_symbol((ctx), (symbol))) { \
            return (ret);                                                 \
        }                                                                 \
    } while (0)

#define EXPECT_QUOTED_STRING_OR(ctx, quote, ret)                                \
    ({                                                                          \
        OptionalToken _t = parser_context_expect_quoted_string((ctx), (quote)); \
        if (!_t.has_value) {                                                    \
            return (ret);                                                       \
        }                                                                       \
        _t.value;                                                               \
    })

#define EXPECT_QUOTED_STRING(ctx, quote) EXPECT_QUOTED_STRING_OR(ctx, quote, NULL)

#define ACCEPT_SYMBOL_AND(ctx, symbol, ret)                              \
    do {                                                                 \
        if (parser_context_accept_and_discard_symbol((ctx), (symbol))) { \
            return (ret);                                                \
        }                                                                \
    } while (0)
#define ACCEPT_KEYWORD_OR(ctx, kw, ret)                                \
    do {                                                               \
        if (!parser_context_accept_and_discard_keyword((ctx), (kw))) { \
            return (ret);                                              \
        }                                                              \
    } while (0)
#define ACCEPT_KEYWORD_AND(ctx, kw, ret)                              \
    do {                                                              \
        if (parser_context_accept_and_discard_keyword((ctx), (kw))) { \
            return (ret);                                             \
        }                                                             \
    } while (0)

#define SKIP_SEMICOLON(ctx) EXPECT_SYMBOL(ctx, ';')

static OperatorMapping s_operator_mapping[] = {
#undef ENUM_BINARY_OPERATOR
#define ENUM_BINARY_OPERATOR(op, a, p, k, c, cl) { OP_##op, true, k, c, (char) cl, p },
    BINARY_OPERATORS(ENUM_BINARY_OPERATOR)
#undef ENUM_BINARY_OPERATOR
#undef ENUM_UNARY_OPERATOR
#define ENUM_UNARY_OPERATOR(op, k, c) { OP_##op, false, k, c, (char) 0, -1 },
        UNARY_OPERATORS(ENUM_UNARY_OPERATOR)
#undef ENUM_BINARY_OPERATOR
            { OP_COUNT, false, TK_UNKNOWN, 0, -1 }
};

OperatorMapping operator_for_token(Token token, bool binary)
{
    for (int ix = 0; s_operator_mapping[ix].operator!= OP_COUNT; ++ix) {
        if (token.kind != s_operator_mapping[ix].token_kind || s_operator_mapping[ix].binary != binary) {
            continue;
        }
        if (token.kind == TK_SYMBOL && token.symbol == s_operator_mapping[ix].token_code) {
            return s_operator_mapping[ix];
        }
        if (token.kind == TK_KEYWORD && token.keyword_code == s_operator_mapping[ix].token_code) {
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
SyntaxNode *parse_expression(ParserContext *ctx)
{
    SyntaxNode *primary = parse_primary_expression(ctx);
    if (!primary)
        return NULL;
    return parse_expression_1(ctx, primary, 0);
}

SyntaxNode *parse_expression_1(ParserContext *ctx, SyntaxNode *lhs, int min_precedence)
{
    Token           lookahead = lexer_next(ctx->lexer);
    OperatorMapping op = operator_for_token(lookahead, true);
    while (op.binary && op.precedence >= min_precedence) {
        lexer_lex(ctx->lexer);

        SyntaxNode *rhs = NULL;
        int         prec = op.precedence;
        if (op.operator== OP_CAST) {
            rhs = parse_type(ctx);
        } else {
            rhs = parse_primary_expression(ctx);
        }
        lookahead = lexer_next(ctx->lexer);
        OperatorMapping op_1 = operator_for_token(lookahead, true);
        while (op_1.binary && op_1.precedence > prec) {
            rhs = parse_expression_1(ctx, rhs, prec + 1);
            if (!rhs) {
                return NULL;
            }
            lookahead = lexer_next(ctx->lexer);
            op_1 = operator_for_token(lookahead, true);
        }
        if (op.operator== OP_SUBSCRIPT) {
            EXPECT_SYMBOL_OR(ctx, ']', NULL);
        }
        if (op.operator== OP_TERNARY) {
            if (rhs->type != SNT_BINARYEXPRESSION || rhs->binary_expr.operator!= OP_TERNARY_ELSE) {
                parser_context_add_error(ctx, rhs->token, "Expected ':' in ternary expression");
                return NULL;
            }
        }
        SyntaxNode *expr = syntax_node_make(ctx, SNT_BINARYEXPRESSION, sv_from(Operator_name(op.operator)), lhs->token);
        expr->binary_expr.lhs = lhs;
        expr->binary_expr.rhs = rhs;
        expr->binary_expr.operator= op.operator;
        lhs = expr;
        lookahead = lexer_next(ctx->lexer);
        op = operator_for_token(lookahead, true);
    }
    return lhs;
}

SyntaxNode *parse_primary_expression(ParserContext *ctx)
{
    Token token = lexer_next(ctx->lexer);
    switch (token.kind) {
    case TK_IDENTIFIER: {
        lexer_lex(ctx->lexer);
        SyntaxNode  *var = syntax_node_make(ctx, SNT_VARIABLE, token.text, token);
        SyntaxNode **name_part = &var->variable.subscript;
        while (parser_context_accept_and_discard_symbol(ctx, '.')) {
            token = EXPECT_IDENTIFIER(ctx);
            *name_part = syntax_node_make(ctx, SNT_VARIABLE, token.text, token);
            name_part = &(*name_part)->variable.subscript;
        }
        ACCEPT_SYMBOL_OR(ctx, '(', var);
        SyntaxNode *call = syntax_node_make(ctx, SNT_FUNCTION_CALL, var->name, var->token);
        var->type = SNT_FUNCTION;
        call->call.function = var;
        call->call.discard_result = false;
        if (!parse_expression_list(ctx, &call->call.arguments, ')')) {
            return NULL;
        }
        return call;
    }
    case TK_NUMBER: {
        lexer_lex(ctx->lexer);
        switch (token.number_type) {
        case NTInteger: {
            SyntaxNode *ret = syntax_node_make(ctx, SNT_INTEGER, token.text, token);
            ret->integer.type = I32;
            Token type = lexer_next(ctx->lexer);
            if (token_matches_kind(type, TK_IDENTIFIER)) {
                char cstr[type.text.length + 1];
                sv_cstr(type.text, cstr);
                IntegerType integer_type = IntegerType_from_name(cstr);
                if (integer_type != IU_NO_SUCH_TYPE) {
                    lexer_lex(ctx->lexer);
                    ret->integer.type = integer_type;
                }
            }
            return ret;
        }
        case NTHexNumber: {
            SyntaxNode *ret = syntax_node_make(ctx, SNT_INTEGER, token.text, token);
            ret->integer.type = (IntegerType) align_at(4 * (token.text.length - 2), 8);
            return ret;
        }
        case NTDecimal: {
            return syntax_node_make(ctx, SNT_DECIMAL, token.text, token);
        }
        default:
            UNREACHABLE();
        }
    }
    case TK_QUOTED_STRING:
        switch (token.quoted_string.quote_type) {
        case QTDoubleQuote:
            lexer_lex(ctx->lexer);
            return syntax_node_make(ctx, SNT_STRING, sv_decode_quoted_str(token.text), token);
        default:
            return NULL;
        }
    case TK_KEYWORD:
        switch (token.keyword_code) {
        case KW_TRUE:
        case KW_FALSE:
            lexer_lex(ctx->lexer);
            return syntax_node_make(ctx, SNT_BOOL, token.text, token);
        default:
            return NULL;
        }
    case TK_SYMBOL:
        switch (token.symbol) {
        case '(': {
            lexer_lex(ctx->lexer);
            SyntaxNode *ret = parse_expression(ctx);
            if (!parser_context_expect_symbol(ctx, ')')) {
                return NULL;
            }
            return ret;
        }
        case '{': {
            lexer_lex(ctx->lexer);
            SyntaxNode *ret = syntax_node_make(ctx, SNT_COMPOUND, token.text, token);
            if (!parse_expression_list(ctx, &ret->compound_expr.expressions, '}')) {
                return NULL;
            }
            return ret;
        }
        default: {
            OperatorMapping op = operator_for_token(token, false);
            if (op.operator!= OP_INVALID) {
                lexer_lex(ctx->lexer);
                SyntaxNode *operand = parse_primary_expression(ctx);
                if (operand) {
                    SyntaxNode *unary_expr = syntax_node_make(ctx, SNT_UNARYEXPRESSION, token.text, token);
                    unary_expr->unary_expr.operand = operand;
                    unary_expr->unary_expr.operator= op.operator;
                    return unary_expr;
                }
            }
            return NULL;
        }
        }
    default:
        return NULL;
    }
}

bool parse_expression_list(ParserContext *ctx, SyntaxNode **dst, char end)
{
    ACCEPT_SYMBOL_AND(ctx, end, true);
    while (true) {
        SyntaxNode *arg = parse_expression(ctx);
        if (!arg) {
            return false;
        }
        (*dst) = arg;
        dst = &(*dst)->next;
        ACCEPT_SYMBOL_AND(ctx, end, true);
        EXPECT_SYMBOL_OR(ctx, ',', false);
    }
}

SyntaxNode *parse_identifier(ParserContext *ctx)
{
    Token        token = lexer_lex(ctx->lexer);
    SyntaxNode  *var = syntax_node_make(ctx, SNT_VARIABLE, token.text, token);
    SyntaxNode **name_part = &var->variable.subscript;
    SyntaxNode  *ret = NULL;
    while (parser_context_accept_and_discard_symbol(ctx, '.')) {
        token = EXPECT_IDENTIFIER(ctx);
        *name_part = syntax_node_make(ctx, SNT_VARIABLE, token.text, token);
        name_part = &(*name_part)->variable.subscript;
    }
    if (parser_context_accept_and_discard_symbol(ctx, '(')) {
        ret = syntax_node_make(ctx, SNT_FUNCTION_CALL, var->name, var->token);
        var->type = SNT_FUNCTION;
        ret->call.function = var;
        ret->call.discard_result = true;
        if (!parse_expression_list(ctx, &ret->call.arguments, ')')) {
            return NULL;
        }
    } else if (parser_context_accept_and_discard_symbol(ctx, '=')) {
        ret = syntax_node_make(ctx, SNT_ASSIGNMENT, var->name, var->token);
        ret->assignment.variable = var;
        ret->assignment.expression = parse_expression(ctx);
        if (ret->assignment.expression == NULL) {
            return NULL;
        }
    } else if (parser_context_accept_and_discard_symbol(ctx, ':')) {
        ret = syntax_node_make(ctx, SNT_LABEL, var->name, var->token);
        return ret;
    }
    SKIP_SEMICOLON(ctx);
    return ret;
}

SyntaxNode *parse_variable_declaration(ParserContext *ctx, bool is_const)
{
    Token       var = lexer_lex(ctx->lexer);
    SyntaxNode *type = NULL;

    Token       ident = EXPECT_IDENTIFIER(ctx);
    SyntaxNode *ret = syntax_node_make(ctx, SNT_VARIABLE_DECL, ident.text, var);
    ret->variable_decl.variable = syntax_node_make(ctx, SNT_VARIABLE, ident.text, ident);
    if (parser_context_accept_and_discard_symbol(ctx, ':')) {
        type = parse_type(ctx);
    }
    ret->variable_decl.var_type = type;
    if (parser_context_accept_and_discard_symbol(ctx, '=')) {
        ret->variable_decl.init_expr = parse_expression(ctx);
        if (ret->variable_decl.init_expr == NULL) {
            return NULL;
        }
    } else if (is_const) {
        parser_context_add_error(ctx, ident, "'const' declaration without initializer expression");
        return NULL;
    }
    ret->variable_decl.is_const = is_const;
    SKIP_SEMICOLON(ctx);
    return ret;
}

SyntaxNode *parse_if(ParserContext *ctx)
{
    Token       token = lexer_lex(ctx->lexer);
    SyntaxNode *expr = parse_expression(ctx);
    if (!expr) {
        parser_context_add_error(ctx, token, "Expected condition in 'if' statement");
        return NULL;
    }
    SyntaxNode *if_true = parse_statement(ctx);
    if (!if_true) {
        parser_context_add_error(ctx, token, "Expected 'true' branch for 'if' statement");
        return NULL;
    }
    SyntaxNode *if_false = NULL;
    if (parser_context_accept_and_discard_keyword(ctx, KW_ELSE)) {
        if_false = parse_statement(ctx);
    }
    SyntaxNode *ret = syntax_node_make(ctx, SNT_IF, sv_from("if"), token);
    ret->if_statement.condition = expr;
    ret->if_statement.if_true = if_true;
    ret->if_statement.if_false = if_false;
    return ret;
}

SyntaxNode *parse_for(ParserContext *ctx)
{
    Token token = lexer_lex(ctx->lexer);
    Token variable = EXPECT_IDENTIFIER(ctx);
    EXPECT_KEYWORD(ctx, KW_IN);
    SyntaxNode *range = parse_expression(ctx);
    if (!range) {
        parser_context_add_error(ctx, token, "Expected range expression in 'for' statement");
        return NULL;
    }
    SyntaxNode *stmt = parse_statement(ctx);
    if (!stmt) {
        parser_context_add_error(ctx, token, "Expected statement for 'for' loop");
        return NULL;
    }
    SyntaxNode *ret = syntax_node_make(ctx, SNT_FOR, sv_from("$for"), token);
    ret->for_statement.variable = variable.text;
    ret->for_statement.range = range;
    ret->for_statement.statement = stmt;
    return ret;
}

SyntaxNode *parse_return(ParserContext *ctx)
{
    Token       token = lexer_lex(ctx->lexer);
    SyntaxNode *ret = syntax_node_make(ctx, SNT_RETURN, sv_from("return"), token);
    ACCEPT_SYMBOL_AND(ctx, ';', ret);
    ret->return_stmt.expression = parse_expression(ctx);
    if (ret->return_stmt.expression == NULL) {
        return NULL;
    }
    SKIP_SEMICOLON(ctx);
    return ret;
}

SyntaxNode *parse_while(ParserContext *ctx)
{
    Token       token = lexer_lex(ctx->lexer);
    SyntaxNode *expr = parse_expression(ctx);
    if (!expr) {
        parser_context_add_error(ctx, token, "Expected condition in 'while' statement");
        return NULL;
    }
    SyntaxNode *stmt = parse_statement(ctx);
    if (!stmt) {
        parser_context_add_error(ctx, token, "Expected statement for 'while' loop");
        return NULL;
    }
    SyntaxNode *ret = syntax_node_make(ctx, SNT_WHILE, sv_from("$while"), token);
    ret->while_statement.condition = expr;
    ret->while_statement.statement = stmt;
    return ret;
}

SyntaxNode *parse_loop(ParserContext *ctx)
{
    Token       token = lexer_lex(ctx->lexer);
    SyntaxNode *stmt = parse_statement(ctx);
    if (!stmt) {
        parser_context_add_error(ctx, token, "Expected statement for loop");
        return NULL;
    }
    SyntaxNode *ret = syntax_node_make(ctx, SNT_LOOP, sv_from("$loop"), token);
    ret->block.statements = stmt;
    return ret;
}

SyntaxNode *parse_block(ParserContext *ctx)
{
    Token        token = lexer_lex(ctx->lexer);
    SyntaxNode  *ret = syntax_node_make(ctx, SNT_BLOCK, sv_from("block"), token);
    SyntaxNode **dst = &ret->block.statements;
    while (true) {
        ACCEPT_SYMBOL_AND(ctx, '}', ret);
        if (parser_context_accept_and_discard(ctx, TK_END_OF_FILE)) {
            parser_context_add_error(ctx, token, "Expected '}' to close block");
            return NULL;
        }
        SyntaxNode *stmt = parse_statement(ctx);
        if (stmt == NULL) {
            return NULL;
        }
        *dst = stmt;
        dst = &stmt->next;
    }
}

SyntaxNode *parse_statement(ParserContext *ctx)
{
    Token       token = lexer_next(ctx->lexer);
    SyntaxNode *ret;
    switch (token.kind) {
    case TK_SYMBOL: {
        switch (token.symbol) {
        case '{':
            return parse_block(ctx);
        default:
            parser_context_add_error(ctx, token, "Unexpected symbol '%c'", (char) token.symbol);
            return NULL;
        }
    }
    case TK_KEYWORD: {
        switch (token.keyword_code) {
        case KW_BREAK:
        case KW_CONTINUE: {
            Token stmt_token = lexer_lex(ctx->lexer);
            token = EXPECT_IDENTIFIER(ctx);
            ret = syntax_node_make(ctx, (stmt_token.keyword_code == KW_BREAK) ? SNT_BREAK : SNT_CONTINUE, token.text, stmt_token);
            SKIP_SEMICOLON(ctx);
        } break;
        case KW_CONST:
            ret = parse_variable_declaration(ctx, true);
            break;
        case KW_IF:
            ret = parse_if(ctx);
            break;
        case KW_FOR:
            ret = parse_for(ctx);
            break;
        case KW_LOOP:
            ret = parse_loop(ctx);
            break;
        case KW_RETURN:
            ret = parse_return(ctx);
            break;
        case KW_VAR:
            ret = parse_variable_declaration(ctx, false);
            break;
        case KW_WHILE:
            ret = parse_while(ctx);
            break;
        default:
            NYI("Keywords");
        }
        break;
    }
    case TK_IDENTIFIER:
        ret = parse_identifier(ctx);
        break;
    default:
        parser_context_add_error(ctx, token, "Unexpected token '%.*s'", SV_ARG(token.text));
        return NULL;
    }
    return ret;
}

bool parse_type_descr(ParserContext *ctx, Token type_name, TypeDescr *target)
{
    target->name = type_name.text;
    if (parser_context_accept_and_discard_symbol(ctx, '<')) {
        while (true) {
            Token      token = EXPECT_IDENTIFIER(ctx);
            TypeDescr *component = MALLOC(TypeDescr);
            DIA_APPEND(TypeDescr *, target, component);
            if (!parse_type_descr(ctx, token, component)) {
                return false;
            }
            if (parser_context_accept_and_discard_symbol(ctx, '>')) {
                return true;
            }
            if (!parser_context_expect_symbol(ctx, ',')) {
                return false;
            }
        }
    }
    return true;
}

SyntaxNode *parse_type(ParserContext *ctx)
{
    Token       type_name = EXPECT_IDENTIFIER(ctx);
    SyntaxNode *ret = syntax_node_make(ctx, SNT_TYPE, type_name.text, type_name);
    if (!parse_type_descr(ctx, type_name, &ret->type_descr)) {
        return NULL;
    }
    return ret;
}

SyntaxNode *parse_param(ParserContext *ctx)
{
    Token name = EXPECT_IDENTIFIER(ctx);
    EXPECT_SYMBOL(ctx, ':');
    SyntaxNode *type = parse_type(ctx);
    if (!type) {
        return NULL;
    }
    SyntaxNode *param = syntax_node_make(ctx, SNT_PARAMETER, name.text, name);
    param->parameter.parameter_type = type;
    return param;
}

SyntaxNode *parse_parameters(ParserContext *ctx, SyntaxNode *func)
{
    if (!parser_context_expect_symbol(ctx, '(')) {
        return NULL;
    }
    if (parser_context_accept_and_discard_symbol(ctx, ')')) {
        return func;
    }
    SyntaxNode *last_param = NULL;
    while (true) {
        SyntaxNode *param = parse_param(ctx);
        if (!param) {
            return NULL;
        }
        if (!last_param) {
            func->function.parameter = param;
        } else {
            last_param->next = param;
        }
        last_param = param;
        if (parser_context_accept_and_discard_symbol(ctx, ')')) {
            return func;
        }
        EXPECT_SYMBOL(ctx, ',');
    }
}

SyntaxNode *parse_return_types(ParserContext *ctx, SyntaxNode *func)
{
    EXPECT_SYMBOL(ctx, ':');
    func->function.return_type = parse_type(ctx);
    if (!func->function.return_type) {
        return NULL;
    }
    ACCEPT_SYMBOL_OR(ctx, '/', func);
    func->function.error_type = parse_type(ctx);
    if (!func->function.error_type) {
        return NULL;
    }
    return func;
}

SyntaxNode *parse_function_decl(ParserContext *ctx)
{
    lexer_lex(ctx->lexer);
    Token       token = EXPECT_IDENTIFIER(ctx);
    SyntaxNode *func = syntax_node_make(ctx, SNT_FUNCTION, token.text, token);
    if (parse_parameters(ctx, func) == NULL) {
        return NULL;
    }
    if (parse_return_types(ctx, func) == NULL) {
        return NULL;
    }
    return func;
}

SyntaxNode *parse_function(ParserContext *ctx)
{
    SyntaxNode *func = parse_function_decl(ctx);
    if (!func) {
        return NULL;
    }
    if (parser_context_accept_and_discard_symbol(ctx, '{')) {
        SyntaxNode *impl = syntax_node_make(ctx, SNT_FUNCTION_IMPL, func->name, func->token);
        func->function.function_impl = impl;
        SyntaxNode *last_stmt = NULL;
        while (true) {
            if (parser_context_accept_and_discard_symbol(ctx, '}')) {
                return func;
            }
            SyntaxNode *stmt = parse_statement(ctx);
            if (!stmt) {
                while (!parser_context_accept_and_discard_symbol(ctx, ';') && !parser_context_accept_and_discard_symbol(ctx, '}') && !parser_context_accept_and_discard(ctx, TK_END_OF_FILE)) {
                    lexer_lex(ctx->lexer);
                }
                continue;
            }
            if (last_stmt == NULL) {
                impl->function_impl.statements = stmt;
            } else {
                last_stmt->next = stmt;
            }
            last_stmt = stmt;
        }
    }
    if (parser_context_accept_and_discard_keyword(ctx, KW_FUNC_BINDING)) {
        Token const token = EXPECT_QUOTED_STRING(ctx, QTDoubleQuote);
        SKIP_SEMICOLON(ctx);
        func->function.function_impl = syntax_node_make(ctx,
            SNT_NATIVE_FUNCTION,
            (StringView) { token.text.ptr + 1, token.text.length - 2 },
            token);
        return func;
    }
    if (parser_context_accept_and_discard_keyword(ctx, KW_MACRO_BINDING)) {
        Token const token = EXPECT_QUOTED_STRING(ctx, QTDoubleQuote);
        SKIP_SEMICOLON(ctx);
        func->function.function_impl = syntax_node_make(ctx,
            SNT_MACRO,
            (StringView) { token.text.ptr + 1, token.text.length - 2 },
            token);
        return func;
    }
    parser_context_add_error(ctx, func->token, "Expected '{', '->', or '=>' after function declaration");
    return NULL;
}

SyntaxNode *parse_enum_def(ParserContext *ctx)
{
    lexer_lex(ctx->lexer);
    Token       ident = EXPECT_IDENTIFIER(ctx);
    SyntaxNode *underlying_type = NULL;
    if (parser_context_accept_and_discard_symbol(ctx, ':')) {
        if ((underlying_type = parse_type(ctx)) == NULL) {
            return NULL;
        }
    }
    EXPECT_SYMBOL(ctx, '{');
    SyntaxNode *enum_node = syntax_node_make(ctx, SNT_ENUMERATION, ident.text, ident);
    enum_node->enumeration.underlying_type = underlying_type;
    SyntaxNode **value = &enum_node->enumeration.values;
    while (!parser_context_accept_and_discard_symbol(ctx, '}')) {
        Token value_name = EXPECT_IDENTIFIER(ctx);
        *value = syntax_node_make(ctx, SNT_ENUM_VALUE, value_name.text, value_name);
        SyntaxNode *underlying_value = NULL;
        if (parser_context_accept_and_discard_symbol(ctx, '=')) {
            if ((underlying_value = parse_expression(ctx)) == NULL) {
                return NULL;
            }
        }
        (*value)->enum_value.underlying_value = underlying_value;
        if (parser_context_accept_and_discard_symbol(ctx, '}')) {
            break;
        }
        EXPECT_SYMBOL(ctx, ',');
        value = &(*value)->next;
    }
    SKIP_SEMICOLON(ctx);
    return enum_node;
}

SyntaxNode *parse_struct_def(ParserContext *ctx)
{
    lexer_lex(ctx->lexer);
    Token ident = EXPECT_IDENTIFIER(ctx);
    EXPECT_SYMBOL(ctx, '{');
    SyntaxNode  *strukt = syntax_node_make(ctx, SNT_STRUCT, ident.text, ident);
    SyntaxNode **comp = &strukt->struct_def.components;
    while (!parser_context_accept_and_discard_symbol(ctx, '}')) {
        Token comp_name = EXPECT_IDENTIFIER(ctx);
        EXPECT_SYMBOL(ctx, ':');
        *comp = syntax_node_make(ctx, SNT_TYPE_COMPONENT, comp_name.text, comp_name);
        if (((*comp)->parameter.parameter_type = parse_type(ctx)) == NULL) {
            return NULL;
        }
        if (parser_context_accept_and_discard_symbol(ctx, '}')) {
            break;
        }
        EXPECT_SYMBOL(ctx, ',');
        comp = &(*comp)->next;
    }
    SKIP_SEMICOLON(ctx);
    return strukt;
}

SyntaxNode *parse_variant_def(ParserContext *ctx)
{
    lexer_lex(ctx->lexer);
    Token       ident = EXPECT_IDENTIFIER(ctx);
    SyntaxNode *underlying_type = NULL;
    if (parser_context_accept_and_discard_symbol(ctx, ':')) {
        underlying_type = parse_type(ctx);
    }
    EXPECT_SYMBOL(ctx, '{');
    SyntaxNode *variant_node = syntax_node_make(ctx, SNT_VARIANT, ident.text, ident);
    variant_node->variant_def.underlying_type = underlying_type;
    SyntaxNode **value = &variant_node->variant_def.options;
    while (!parser_context_accept_and_discard_symbol(ctx, '}')) {
        Token option_name = EXPECT_IDENTIFIER(ctx);
        *value = syntax_node_make(ctx, SNT_VARIANT_OPTION, option_name.text, option_name);
        SyntaxNode *underlying_value = NULL;
        SyntaxNode *payload_type = NULL;
        if (parser_context_accept_and_discard_symbol(ctx, '(')) {
            if ((payload_type = parse_type(ctx)) == NULL) {
                return NULL;
            }
            EXPECT_SYMBOL(ctx, ')');
        }
        if (parser_context_accept_and_discard_symbol(ctx, '=')) {
            if ((underlying_value = parse_expression(ctx)) == NULL) {
                return NULL;
            }
        }
        (*value)->variant_option.underlying_value = underlying_value;
        (*value)->variant_option.payload_type = payload_type;
        if (parser_context_accept_and_discard_symbol(ctx, '}')) {
            break;
        }
        EXPECT_SYMBOL(ctx, ',');
        value = &(*value)->next;
    }
    SKIP_SEMICOLON(ctx);
    return variant_node;
}

SyntaxNode *import_package(ParserContext *ctx, Token token, StringView path)
{
    StringView name = sv_replace(path, sv_from("/"), sv_from("."));
    StringView file_name = sv_printf("%.*s.scribble", SV_ARG(path));
    if (!fs_file_exists(file_name)) {
        StringView scribble_dir = sv_from(getenv("SCRIBBLE_DIR"));
        if (sv_empty(scribble_dir) || !fs_file_exists(scribble_dir)) {
            scribble_dir = sv_from(SCRIBBLE_DIR);
        }
        if (sv_empty(scribble_dir) || !fs_file_exists(scribble_dir)) {
            scribble_dir = sv_from("/usr/local/scribble");
        }
        if (sv_empty(scribble_dir) || !fs_file_exists(scribble_dir)) {
            fatal("Broken scribble installation");
        }
        file_name = sv_printf("%.*s/share/%.*s", SV_ARG(scribble_dir), SV_ARG(file_name));
    }
    if (!fs_file_exists(file_name)) {
        parser_context_add_error(ctx, token, "Could not find import '%.*s'", SV_ARG(path));
        return NULL;
    }
    SyntaxNode       *import = syntax_node_make(ctx, SNT_IMPORT, name, token);
    ErrorOrStringView buffer_maybe = read_file_by_name(file_name);
    if (ErrorOrStringView_is_error(buffer_maybe)) {
        parser_context_add_error(ctx, token, "Could not read import '%.*s'", SV_ARG(path));
        parser_context_add_note(ctx, token, buffer_maybe.error.message);
        return NULL;
    }
    StringView  buffer = buffer_maybe.value;
    SyntaxNode *module = parse_module(ctx, buffer, name);
    import->next = ctx->program->program.imports;
    ctx->program->program.imports = import;
    module->next = import->import.modules;
    import->import.modules = module;
    return import;
}

SyntaxNode *parse_import(ParserContext *ctx)
{
    Token      token = lexer_lex(ctx->lexer);
    Token      name = EXPECT_QUOTED_STRING(ctx, QTDoubleQuote);
    StringView path = sv_decode_quoted_str(name.text);
    SKIP_SEMICOLON(ctx);
    return import_package(ctx, token, path);
}

static int handle_include_directive(Lexer *lexer)
{
    char const *buffer = lexer_source(lexer).ptr;
    size_t      ix = 0;
    size_t      state = (size_t) lexer->language_data;
    switch (state) {
    case ScribbleDirectiveStateInit: {
        while (buffer[ix] == ' ' || buffer[ix] == '\t') {
            ++ix;
        }
        lexer->language_data = (void *) ScribbleDirectiveStateIncludeQuote;
        if (ix > 0) {
            lexer_set_current(lexer, (Token) { .kind = TK_WHITESPACE, .text = { buffer, ix } });
            return ScribbleDirectiveInclude;
        }
        lexer->language_data = (void *) ScribbleDirectiveStateIncludeQuote;
    } // Fall through
    case ScribbleDirectiveStateIncludeQuote: {
        if (buffer[0] != '<' && buffer[0] != '"') {
            lexer->language_data = NULL;
            return NO_DIRECTIVE;
        }
        ++ix;
        while (buffer[ix] && buffer[ix] != '"' && buffer[ix] != '\n') {
            ++ix;
        }
        if (buffer[ix] == '"') {
            lexer_set_current(lexer, (Token) { .kind = TK_DIRECTIVE_ARG, .text = { buffer, ix + 1 } });
        } else {
            lexer_set_current(lexer, (Token) { .kind = TK_DIRECTIVE_ARG, .text = { buffer, ix } });
        }
    } break;
    default:
        UNREACHABLE();
    }
    lexer->language_data = NULL;
    return NO_DIRECTIVE;
}

int handle_scribble_directive(Lexer *lexer, int directive)
{
    switch (directive) {
    case ScribbleDirectiveInclude:
        return handle_include_directive(lexer);
    default:
        lexer->language_data = (void *) NULL;
        return NO_DIRECTIVE;
    }
}

SyntaxNode *parse_module(ParserContext *ctx, StringView buffer, StringView name)
{
    Token       token = { .kind = TK_MODULE, .text = name };
    SyntaxNode *module = syntax_node_make(ctx, SNT_MODULE, name, token);
    Lexer       lexer = lexer_for_language(&scribble_language);
    lexer.whitespace_significant = false;
    lexer.include_comments = false;

    parser_debug_info(ctx, "Compiling '%.*s'", SV_ARG(name));
    ctx->lexer = &lexer;
    lexer_push_source(&lexer, buffer, name);
    SyntaxNode *last_statement = NULL;
    while (true) {
        token = lexer_next(&lexer);

        SyntaxNode *statement = NULL;
        switch (token.kind) {
        case TK_KEYWORD: {
            switch (token.keyword_code) {
            case KW_FUNC: {
                statement = parse_function(ctx);
                if (statement) {
                    trace(PARSE, "Function '%.*s' parsed", SV_ARG(statement->name));
                }
            } break;
            case KW_ENUM:
                statement = parse_enum_def(ctx);
                break;
            case KW_STRUCT:
                statement = parse_struct_def(ctx);
                break;
            case KW_VARIANT:
                statement = parse_variant_def(ctx);
                break;
            case KW_IMPORT:
                statement = parse_import(ctx);
                break;
            case KW_CONST:
                statement = parse_variable_declaration(ctx, true);
                break;
            case KW_VAR:
                statement = parse_variable_declaration(ctx, false);
                break;
            default:
                break;
            }
        } break;
        case TK_END_OF_FILE:
            return module;
        default:
            break;
        }
        if (statement) {
            if (!last_statement) {
                module->block.statements = statement;
            } else {
                last_statement->next = statement;
            }
            last_statement = statement;
        } else {
            parser_context_add_error(ctx, token, "Only 'import', 'func', 'var', 'const', and 'struct' are allowed on the top level of files, '%.*s' is not", SV_ARG(token.text));
            while (true) {
                lexer_lex(&lexer);
                token = lexer_next(&lexer);
                if (token.kind == TK_KEYWORD && (token.keyword_code == KW_IMPORT || token.keyword_code == KW_FUNC || token.keyword_code == KW_STRUCT || token.keyword_code == KW_VAR || token.keyword_code == KW_CONST)) {
                    break;
                }
                if (token.kind == TK_END_OF_FILE) {
                    return module;
                }
            }
        }
    next:
    }
    return module;
}

SyntaxNode *parse_module_file(ParserContext *ctx, int dir_fd, StringView file)
{
    StringView buffer = TRY_OR_NULL(StringView, read_file_at(dir_fd, file));
    return parse_module(ctx, buffer, fn_barename(file));
}

void parser_debug_info(ParserContext *ctx, char const *fmt, ...)
{
    if (!ctx->debug)
        return;
    va_list args;
    va_start(args, fmt);
    HTTP_POST_MUST(ctx->frontend, "/parser/info", json_string(sv_vprintf(fmt, args)));
    va_end(args);
}

void parser_debug_node(ParserContext *ctx, SyntaxNode *node)
{
    if (!ctx || !ctx->debug)
        return;
    JSONValue n = json_object();
    json_set_string(&n, "name", node->name);
    json_set_cstr(&n, "type", SyntaxNodeType_name(node->type));
    json_set(&n, "location", location_to_json(node->token.location));
    HTTP_POST_MUST(ctx->frontend, "/parser/node", n);
}

ParserContext parse_text(BackendConnection *conn, JSONValue config)
{
    assert(json_has(&config, "text"));
    StringView text = json_get_string(&config, "text", sv_null());
    StringView name = json_get_string(&config, "buffer_name", SV("untitled"));
    ParserContext ret = { 0 };
    ret.frontend = conn->fd;
    ret.debug = json_get_bool(&config, "debug", false);
    ret.source_name = sv_copy(name);

    if (ret.debug) {
        HTTP_POST_MUST(conn->fd, "/parser/start", json_string(name));
    }

    Token token = { .kind = TK_PROGRAM, .text = ret.source_name };
    ret.program = syntax_node_make(&ret, SNT_PROGRAM, ret.source_name, token);
    if (json_get_bool(&config, "stdlib", true)) {
        import_package(&ret, token, SV("std"));
    }
    OptionalJSONValue libs_maybe = json_get(&config, "libraries");
    if (libs_maybe.has_value && libs_maybe.value.type == JSON_TYPE_ARRAY) {
        JSONValue libs = libs_maybe.value;
        for (size_t ix = 0; ix < json_len(&libs); ++ix) {
            JSONValue lib = MUST_OPTIONAL(JSONValue, json_at(&libs, ix));
            if (lib.type == JSON_TYPE_STRING) {
                import_package(&ret, token, lib.string);
            }
        }
    }
    SyntaxNode *module = parse_module(&ret, text, ret.source_name);
    module->next = ret.program->program.modules;
    ret.program->program.modules = module;

    if (ret.debug) {
        HTTP_GET_MUST(conn->fd, "/parser/done", sl_create());
    }
    return ret;
}

ParserContext parse(BackendConnection *conn, JSONValue config)
{
    if (json_has(&config, "text")) {
        return parse_text(conn, config);
    }
    StringView dir_or_file = json_get_string(&config, "target", SV("."));

    ParserContext ret = { 0 };
    ret.frontend = conn->fd;
    ret.debug = json_get_bool(&config, "debug", false);
    ret.source_name = sv_copy(dir_or_file);

    if (ret.debug) {
        HTTP_POST_MUST(conn->fd, "/parser/start", json_string(dir_or_file));
        char cwd[256];
        getcwd(cwd, 256);
        parser_debug_info(&ret, "CWD: %s dir: %.*s", cwd, SV_ARG(dir_or_file));
    }

    Token token = { .kind = TK_PROGRAM, .text = ret.source_name };
    ret.program = syntax_node_make(&ret, SNT_PROGRAM, fn_barename(ret.source_name), token);
    import_package(&ret, token, sv_from("std"));

    char dir_cstr[ret.source_name.length + 1];
    sv_cstr(ret.source_name, dir_cstr);
    DIR *dir = opendir(dir_cstr);
    if (dir == NULL) {
        if (errno == ENOTDIR) {
            dir = opendir(".");
            if (dir == NULL) {
                fatal("Could not open current directory");
            }
            SyntaxNode *module = parse_module_file(&ret, dirfd(dir), ret.source_name);
            module->next = ret.program->program.modules;
            ret.program->program.modules = module;
            closedir(dir);
            if (ret.debug) {
                HTTP_GET_MUST(conn->fd, "/parser/done", sl_create());
            }
            return ret;
        }
        fatal("Could not open directory '%.*s'", SV_ARG(dir_or_file));
    }

    struct dirent *dp;
    while ((dp = readdir(dir)) != NULL) {
#ifdef HAVE_DIRENT_D_NAMLEN
        if ((dp->d_namlen > 8) && strcmp(dp->d_name + (dp->d_namlen - 9), ".scribble") == 0) {
#else
        size_t namlen = strlen(dp->d_name);
        if ((namlen > 8) && strcmp(dp->d_name + (namlen - 9), ".scribble") == 0) {
#endif
            SyntaxNode *module = parse_module_file(&ret, dirfd(dir), sv_from(dp->d_name));
            if (!module) {
                continue;
            }
            module->next = ret.program->program.modules;
            ret.program->program.modules = module;
        }
    }
    closedir(dir);
    if (ret.debug) {
        HTTP_GET_MUST(conn->fd, "/parser/done", sl_create());
    }
    return ret;
}
