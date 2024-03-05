/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <ctype.h>

#include <json.h>
#include <template/template.h>

DA_IMPL(Macro);
DA_IMPL(Parameter);
DA_IMPL_TYPE(TemplateExpression, TemplateExpression *);

/*
 * Precedences according to https://en.cppreference.com/w/c/language/operator_precedence
 */
static TemplateOperatorMapping s_operator_mapping[] = {
#undef S
#define S(TOKEN, STR, BINOP, BINPREC, UNOP, UNPREC) { TOT##TOKEN, STR, BINOP, BINPREC, UNOP, UNPREC },
    TEMPLATEOPERATORTOKENS(S) { TOTCount, "X", InvalidOperator, -1, InvalidOperator, -1 },
#undef S
};

#define IS_IDENTIFIER_START(ch) (isalpha(ch) || ch == '$' || ch == '_')
#define IS_IDENTIFIER_CHAR(ch) (isalpha(ch) || isdigit(ch) || ch == '$' || ch == '_')

char const *TemplateExpressionTokenType_name(TemplateExpressionTokenType type)
{
    switch (type) {
#undef S
#define S(T)      \
    case TETT##T: \
        return #T;
        TEMPLATEEXPRESSIONTOKENTYPES(S)
#undef S
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

StringView template_token_to_string(TemplateParserContext *ctx, TemplateExpressionToken token)
{
    size_t current = ctx->sb.length;
    sb_printf(&ctx->sb, "%s ", TemplateExpressionTokenType_name(token.type));
    switch (token.type) {
    case TETTSymbol:
        sb_printf(&ctx->sb, "'%c'", token.ch);
        break;
    case TETTOperator:
        sb_printf(&ctx->sb, "'%s'", TemplateOperatorToken_name(token.op));
        break;
    case TETTUnknown:
    case TETTEndOfText:
        break;
    case TETTString:
    case TETTComment: {
        StringView s = sv(&ctx->sb, token.text);
        sb_printf(&ctx->sb, "'%.*s'", SV_ARG(s));
    } break;
    default:
        sb_printf(&ctx->sb, "'%.*s'", SV_ARG(token.raw_text));
        break;
    }
    return (StringView) { ctx->sb.ptr + current, ctx->sb.length - current };
}

OptionalTemplateOperatorMapping template_operator_mapping(TemplateOperatorToken token)
{
    if (token < TOTCount) {
        return OptionalTemplateOperatorMapping_create(s_operator_mapping[token]);
    }
    return OptionalTemplateOperatorMapping_empty();
}

void template_lexer_consume(TemplateParserContext *ctx)
{
    if (ctx->token.type != TETTEndOfText) {
        trace(CAT_TEMPLATE, "template_lexer_consume");
        ctx->token = (TemplateExpressionToken) { 0 };
        return;
    }
    trace(CAT_TEMPLATE, "template_lexer_consume (ignored; EndOfText)");
}

ErrorOrTemplateExpressionToken template_lexer_peek(TemplateParserContext *ctx)
{
    if (ctx->token.type != TETTUnknown) {
        StringView token_string = template_token_to_string(ctx, ctx->token);
        trace(CAT_TEMPLATE, "template_lexer_next: %.*s (pending)", SV_ARG(token_string));
        RETURN(TemplateExpressionToken, ctx->token);
    }

    StringScanner          *ss = &ctx->ss;
    int                     ch = ss_peek(ss);
    size_t                  current_index = ctx->sb.length;
    TemplateExpressionToken token = { 0 };
    StringView              s = sv_lchop(ss->string, ss->point);

    ss_reset(ss);
    token.type = TETTUnknown;

    if (!ch) {
        token.type = TETTEndOfText;
    } else if (IS_IDENTIFIER_START(ch)) {
        token.type = TETTIdentifier;
        for (ch = ss_peek(ss); IS_IDENTIFIER_CHAR(ch); ch = ss_peek(ss)) {
            ss_skip_one(ss);
        }
        s = ss_read_from_mark(ss);
        if (sv_eq_cstr(s, "true")) {
            token.type = TETTTrue;
        } else if (sv_eq_cstr(s, "false")) {
            token.type = TETTFalse;
        } else if (sv_eq_cstr(s, "null")) {
            token.type = TETTNull;
        }
    } else if (isdigit(ch)) {
        token.type = TETTNumber;
        for (ch = ss_peek(ss); ch && isdigit(ch); ch = ss_peek(ss)) {
            ss_skip_one(ss);
        }
    } else if (isspace(ch)) {
        token.type = TETTWhitespace;
        for (ch = ss_peek(ss); ch && isspace(ch); ch = ss_peek(ss)) {
            ss_skip_one(ss);
        }
    } else if (strchr("\"'`", ch) != NULL) {
        token.type = TETTString;
        int quote = ch;
        ss_skip_one(ss);
        for (ch = ss_peek(ss); ch && ch != quote; ch = ss_peek(ss)) {
            if (ch == '\\') {
                ss_skip_one(ss);
                ch = ss_peek(ss);
                if (!ch) {
                    break;
                }
            }
            sb_append_char(&ctx->sb, ch);
            ss_skip_one(ss);
        }
        if (ch == quote) {
            ss_skip_one(ss);
        }
    } else if (ch == '@') {
        ss_skip_one(ss);
        ch = ss_peek(ss);
        switch (ch) {
        case '%':
            ss_skip_one(ss);
            token.type = TETTStartOfStatement;
            break;
        case '=':
            ss_skip_one(ss);
            token.type = TETTStartOfExpression;
            break;
        case '#':
            token.type = TETTComment;
            ss_skip_one(ss);
            while (!ss_expect_sv(ss, sv_from("#@"))) {
                sb_append_char(&ctx->sb, ss_peek(ss));
                ss_skip_one(ss);
            }
            break;
        default:
            token.type = TETTSymbol;
            token.ch = ch;
            break;
        }
    } else if (ss_expect_sv(ss, sv_from("=@"))) {
        token.type = TETTEndOfExpression;
    } else if (ss_expect_sv(ss, sv_from("%@"))) {
        token.type = TETTEndOfStatement;
    } else if (ss_expect_sv(ss, sv_from("/@"))) {
        token.type = TETTEndOfStatementBlock;
    } else {
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
            token.type = TETTOperator;
            token.op = matched;
        } else {
            token.type = TETTSymbol;
            ss_skip_one(ss);
            token.ch = ch;
        }
    }
    token.raw_text = ss_read_from_mark(ss);
    if (ctx->sb.length - current_index > 0) {
        token.text = (StringRef) { current_index, ctx->sb.length - current_index };
    }
    ctx->token = token;
    assert(token.type != TETTUnknown);

    {
        StringView token_string = template_token_to_string(ctx, token);
        trace(CAT_TEMPLATE, "template_lexer_peek: %.*s", SV_ARG(token_string));
    }
    RETURN(TemplateExpressionToken, token);
}

ErrorOrTemplateExpressionToken template_lexer_next(TemplateParserContext *ctx)
{
    TemplateExpressionToken token;
    while (true) {
        token = TRY(TemplateExpressionToken, template_lexer_peek(ctx));
        if (token.type == TETTComment || token.type == TETTWhitespace) {
            template_lexer_consume(ctx);
            continue;
        }
        break;
    }
    RETURN(TemplateExpressionToken, token);
}

ErrorOrOptionalTemplateExpressionToken template_lexer_allow_type(TemplateParserContext *ctx, TemplateExpressionTokenType type)
{
    TemplateExpressionToken token = TRY_TO(TemplateExpressionToken, OptionalTemplateExpressionToken, template_lexer_next(ctx));
    StringView              t = template_token_to_string(ctx, token);
    if (token.type != type) {
        trace(CAT_TEMPLATE, "Looking for token type '%s', got '%.*s'", TemplateExpressionTokenType_name(type), SV_ARG(t));
        RETURN(OptionalTemplateExpressionToken, OptionalTemplateExpressionToken_empty());
    }
    template_lexer_consume(ctx);
    trace(CAT_TEMPLATE, "Lexed token '%.*s'", SV_ARG(t));
    RETURN(OptionalTemplateExpressionToken, OptionalTemplateExpressionToken_create(token));
}

ErrorOrTemplateExpressionToken template_lexer_require_type(TemplateParserContext *ctx, TemplateExpressionTokenType type)
{
    OptionalTemplateExpressionToken token_maybe = TRY_TO(OptionalTemplateExpressionToken, TemplateExpressionToken, template_lexer_allow_type(ctx, type));
    if (!token_maybe.has_value) {
        ERROR(TemplateExpressionToken, TemplateError, __LINE__, "Required token of type '%s'", TemplateExpressionTokenType_name(type));
    }
    RETURN(TemplateExpressionToken, token_maybe.value);
}

ErrorOrOptionalStringView template_lexer_allow_identifier(TemplateParserContext *ctx)
{
    TemplateExpressionToken token = TRY_TO(TemplateExpressionToken, OptionalStringView, template_lexer_next(ctx));
    if (token.type != TETTIdentifier) {
        StringView t = template_token_to_string(ctx, token);
        trace(CAT_TEMPLATE, "Looking for identifier, got '%.*s'", SV_ARG(t));
        RETURN(OptionalStringView, OptionalStringView_empty());
    }
    template_lexer_consume(ctx);
    trace(CAT_TEMPLATE, "Lexed identifier '%.*s'", SV_ARG(token.raw_text));
    RETURN(OptionalStringView, OptionalStringView_create(token.raw_text));
}

ErrorOrBool template_lexer_allow_sv(TemplateParserContext *ctx, StringView string)
{
    TemplateExpressionToken token = TRY_TO(TemplateExpressionToken, Bool, template_lexer_next(ctx));
    if (token.type != TETTIdentifier) {
        StringView t = template_token_to_string(ctx, token);
        trace(CAT_TEMPLATE, "Looking for '%.*s', got '%.*s'", SV_ARG(string), SV_ARG(t));
        RETURN(Bool, false);
    }
    if (!sv_eq(token.raw_text, string)) {
        trace(CAT_TEMPLATE, "Looking for '%.*s', got '%.*s'", SV_ARG(string), SV_ARG(token.raw_text));
        RETURN(Bool, false);
    }
    template_lexer_consume(ctx);
    trace(CAT_TEMPLATE, "Lexed identifier '%.*s'", SV_ARG(token.raw_text));
    RETURN(Bool, true);
}

ErrorOrStringView template_lexer_require_identifier(TemplateParserContext *ctx)
{
    OptionalStringView identifier_maybe = TRY_TO(OptionalStringView, StringView, template_lexer_allow_identifier(ctx));
    if (!identifier_maybe.has_value) {
        ERROR(StringView, TemplateError, __LINE__, "Required identifier");
    }
    RETURN(StringView, identifier_maybe.value);
}

ErrorOrBool template_lexer_allow_symbol(TemplateParserContext *ctx, int symbol)
{
    TemplateExpressionToken token = TRY_TO(TemplateExpressionToken, Bool, template_lexer_next(ctx));
    if (token.type != TETTSymbol || token.ch != symbol) {
        trace(CAT_TEMPLATE, "Looking for symbol '%c', got %s", symbol, TemplateExpressionTokenType_name(token.type));
        RETURN(Bool, false);
    }
    template_lexer_consume(ctx);
    trace(CAT_TEMPLATE, "Lexed symbol '%c'", symbol);
    RETURN(Bool, true);
}

ErrorOrBool template_lexer_require_symbol(TemplateParserContext *ctx, int symbol)
{
    bool symbol_maybe = TRY(Bool, template_lexer_allow_symbol(ctx, symbol));
    if (!symbol_maybe) {
        ERROR(Bool, TemplateError, __LINE__, "Required symbol '%c'", symbol);
    }
    RETURN(Bool, true);
}

ErrorOrTemplateExpressionToken template_lexer_require_one_of(TemplateParserContext *ctx, char *symbols)
{
    TemplateExpressionToken token = TRY(TemplateExpressionToken, template_lexer_require_type(ctx, TETTSymbol));
    if (strchr(symbols, token.ch) == NULL) {
        ERROR(TemplateExpressionToken, TemplateError, __LINE__, "Required one of '%s'", symbols);
    }
    RETURN(TemplateExpressionToken, token);
}

ErrorOrOptionalTemplateOperatorMapping template_lexer_operator(TemplateParserContext *ctx)
{
    TemplateExpressionToken lookahead = TRY_TO(TemplateExpressionToken, OptionalTemplateOperatorMapping, template_lexer_next(ctx));
    switch (lookahead.type) {
    case TETTOperator:
        trace(CAT_TEMPLATE, "template_lexer_operator: %s", TemplateOperatorToken_name(lookahead.op));
        RETURN(OptionalTemplateOperatorMapping, template_operator_mapping(lookahead.op));
    default:
        trace(CAT_TEMPLATE, "template_lexer_operator: empty");
        RETURN(OptionalTemplateOperatorMapping,
            OptionalTemplateOperatorMapping_empty());
    }
}
