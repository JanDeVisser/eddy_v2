/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "log.h"
#include <ctype.h>

#include <json.h>
#include <template/template.h>

DA_IMPL(Macro);
DA_IMPL(Parameter);
DA_IMPL_TYPE(TemplateExpression, TemplateExpression *);

/*
 * Precedences according to https://en.cppreference.com/w/c/language/operator_precedence
 */
static TplOperatorMapping s_operator_mapping[] = {
#undef S
#define S(TOKEN, STR, BINOP, BINPREC, UNOP, UNPREC) { TO##TOKEN, STR, BINOP, BINPREC, UNOP, UNPREC },
    TPLOPTOKENS(S) { TOCount, "X", InvalidOperator, -1, InvalidOperator, -1 },
#undef S
};

static TplKeywordMapping s_keyword_mapping[] = {
#undef S
#define S(TOKEN, STR, ALT) { TKW##TOKEN, STR, ALT },
    TPLKEYWORDS(S) { TKWCount, NULL, NULL },
#undef S
};

#define IS_IDENTIFIER_START(ch) (isalpha(ch) || ch == '$' || ch == '_')
#define IS_IDENTIFIER_CHAR(ch) (isalpha(ch) || isdigit(ch) || ch == '$' || ch == '_')

char const *TplTokenType_name(TPLTokenType type)
{
    switch (type) {
#undef S
#define S(T)     \
    case TTT##T: \
        return #T;
        TPLTOKENTYPES(S)
#undef S
    default:
        UNREACHABLE();
    }
}

char const *TplOpToken_name(TplOpToken token)
{
    switch (token) {
#undef S
#define S(TOKEN, STR, BINOP, BINPREC, UNOP, UNPREC) \
    case TO##TOKEN:                                 \
        return #TOKEN;
        TPLOPTOKENS(S)
#undef S
    default:
        UNREACHABLE();
    }
}

StringView template_token_to_string(TemplateParserContext *ctx, TplToken token)
{
    size_t current = ctx->sb.length;
    sb_printf(&ctx->sb, "%s ", TplTokenType_name(token.type));
    switch (token.type) {
    case TTTSymbol:
        sb_printf(&ctx->sb, "'%c'", token.ch);
        break;
    case TTTOperator:
        sb_printf(&ctx->sb, "'%s'", TplOpToken_name(token.op));
        break;
    case TTTUnknown:
    case TTTEndOfText:
        break;
    case TTTString:
    case TTTComment: {
        StringView s = sv(&ctx->sb, token.text);
        sb_printf(&ctx->sb, "'%.*s'", SV_ARG(s));
    } break;
    default:
        sb_printf(&ctx->sb, "'%.*s'", SV_ARG(token.raw_text));
        break;
    }
    return (StringView) { ctx->sb.ptr + current, ctx->sb.length - current };
}

OptionalTplOperatorMapping template_operator_mapping(TplOpToken token)
{
    if (token < TOCount) {
        return OptionalTplOperatorMapping_create(s_operator_mapping[token]);
    }
    return OptionalTplOperatorMapping_empty();
}

void template_lexer_consume(TemplateParserContext *ctx)
{
    if (ctx->token.type != TTTEndOfText) {
        ctx->token = (TplToken) { 0 };
        return;
    }
}

ErrorOrTplToken template_lexer_peek(TemplateParserContext *ctx)
{
    if (ctx->token.type != TTTUnknown) {
        StringView token_string = template_token_to_string(ctx, ctx->token);
        trace(CAT_TEMPLATE, "template_lexer_peek: %.*s (pending)", SV_ARG(token_string));
        RETURN(TplToken, ctx->token);
    }

    StringScanner *ss = &ctx->ss;
    int            ch = ss_peek(ss);
    size_t         current_index = ctx->sb.length;
    TplToken       token = { 0 };

    token.position = ss->point;
    ss_reset(ss);

    if (!ch) {
        token.type = TTTEndOfText;
    } else if (IS_IDENTIFIER_START(ch)) {
        token.type = TTTIdentifier;
        for (ch = ss_peek(ss); IS_IDENTIFIER_CHAR(ch); ch = ss_peek(ss)) {
            ss_skip_one(ss);
        }
        StringView s = ss_read_from_mark(ss);
        if (sv_eq_cstr(s, "true")) {
            token.type = TTTTrue;
        } else if (sv_eq_cstr(s, "false")) {
            token.type = TTTFalse;
        } else if (sv_eq_cstr(s, "null")) {
            token.type = TTTNull;
        }
    } else if (isdigit(ch)) {
        token.type = TTTNumber;
        for (ch = ss_peek(ss); ch && isdigit(ch); ch = ss_peek(ss)) {
            ss_skip_one(ss);
        }
    } else if (isspace(ch)) {
        token.type = TTTWhitespace;
        for (ch = ss_peek(ss); ch && isspace(ch); ch = ss_peek(ss)) {
            ss_skip_one(ss);
        }
    } else if (ch == '\\') {
        token.type = TTTSymbol;
        ss_skip_one(ss);
        ch = ss_peek(ss);
        if (!ch) {
            ERROR(TplToken, TemplateError, 0, "Unexpected end of input");
        }
        token.ch = ch;
        ss_skip_one(ss);
    } else if (strchr("\"'`", ch) != NULL) {
        token.type = TTTString;
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
        case '#':
            token.type = TTTComment;
            ss_skip_one(ss);
            while (ss_peek(ss) && !ss_expect_sv(ss, sv_from("#@"))) {
                sb_append_char(&ctx->sb, ss_peek(ss));
                ss_skip_one(ss);
            }
            break;
        default: {
            int        matched = -1;
            StringView matched_kw = { 0 };
            StringView s = ss_peek_tail(ss);
            for (int ix = 0; ix < TKWCount; ++ix) {
                StringView kw = sv_from(s_keyword_mapping[ix].string);
                if (sv_startswith(s, kw)) {
                    if (matched < 0 || sv_length(kw) > sv_length(matched_kw)) {
                        matched = ix;
                        matched_kw = kw;
                    }
                }
            }
            token.type = TTTKeyword;
            if (matched >= 0) {
                ss_skip(ss, matched_kw.length);
                token.keyword = matched;
            } else {
                token.keyword = TKWClose;
            }
        } break;
        }
    } else {
        int        matched = -1;
        StringView matched_op = { 0 };
        StringView s = ss_peek_tail(ss);
        for (int ix = 0; ix < TOCount; ++ix) {
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
            token.type = TTTOperator;
            token.op = matched;
        } else {
            token.type = TTTSymbol;
            ss_skip_one(ss);
            token.ch = ch;
        }
    }
    token.raw_text = ss_read_from_mark(ss);
    if (ctx->sb.length - current_index > 0) {
        token.text = (StringRef) { current_index, ctx->sb.length - current_index };
    }
    ctx->token = token;
    assert(token.type != TTTUnknown);

    {
        StringView token_string = template_token_to_string(ctx, token);
        trace(CAT_TEMPLATE, "template_lexer_peek: %zu:%zu %.*s", token.position.line, token.position.column, SV_ARG(token_string));
    }
    RETURN(TplToken, token);
}

ErrorOrTplToken template_lexer_next(TemplateParserContext *ctx)
{
    TplToken token;
    while (true) {
        token = TRY(TplToken, template_lexer_peek(ctx));
        if (token.type == TTTComment || token.type == TTTWhitespace) {
            template_lexer_consume(ctx);
            continue;
        }
        break;
    }
    RETURN(TplToken, token);
}

ErrorOrOptionalTplToken template_lexer_allow_type(TemplateParserContext *ctx, TPLTokenType type)
{
    TplToken   token = TRY_TO(TplToken, OptionalTplToken, template_lexer_next(ctx));
    StringView t = template_token_to_string(ctx, token);
    if (token.type != type) {
        trace(CAT_TEMPLATE, "Looking for token type '%s', got '%.*s'", TplTokenType_name(type), SV_ARG(t));
        RETURN(OptionalTplToken, OptionalTplToken_empty());
    }
    template_lexer_consume(ctx);
    trace(CAT_TEMPLATE, "Lexed token '%.*s'", SV_ARG(t));
    RETURN(OptionalTplToken, OptionalTplToken_create(token));
}

ErrorOrTplToken template_lexer_require_type(TemplateParserContext *ctx, TPLTokenType type)
{
    OptionalTplToken token_maybe = TRY_TO(OptionalTplToken, TplToken, template_lexer_allow_type(ctx, type));
    if (!token_maybe.has_value) {
        ERROR(TplToken, TemplateError, __LINE__, "Required token of type '%s'", TplTokenType_name(type));
    }
    RETURN(TplToken, token_maybe.value);
}

ErrorOrOptionalStringView template_lexer_allow_identifier(TemplateParserContext *ctx)
{
    TplToken token = TRY_TO(TplToken, OptionalStringView, template_lexer_next(ctx));
    if (token.type != TTTIdentifier) {
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
    TplToken token = TRY_TO(TplToken, Bool, template_lexer_next(ctx));
    if (token.type != TTTIdentifier) {
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
    TplToken token = TRY_TO(TplToken, Bool, template_lexer_next(ctx));
    if (token.type != TTTSymbol || token.ch != symbol) {
        trace(CAT_TEMPLATE, "Looking for symbol '%c', got %s", symbol, TplTokenType_name(token.type));
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

ErrorOrTplToken template_lexer_require_one_of(TemplateParserContext *ctx, char *symbols)
{
    TplToken token = TRY(TplToken, template_lexer_require_type(ctx, TTTSymbol));
    if (strchr(symbols, token.ch) == NULL) {
        ERROR(TplToken, TemplateError, __LINE__, "Required one of '%s'", symbols);
    }
    RETURN(TplToken, token);
}

ErrorOrBool template_lexer_allow_keyword(TemplateParserContext *ctx, TplKeyword keyword)
{
    TplToken token = TRY_TO(TplToken, Bool, template_lexer_next(ctx));
    if (token.type != TTTKeyword || token.keyword != keyword) {
        trace(CAT_TEMPLATE, "Looking for keyword '%s', got %s",
            TplKeyword_name(keyword), TplTokenType_name(token.type));
        RETURN(Bool, false);
    }
    template_lexer_consume(ctx);
    trace(CAT_TEMPLATE, "Lexed keyword '%s'", TplKeyword_name(keyword));
    RETURN(Bool, true);
}

ErrorOrBool template_lexer_require_keyword(TemplateParserContext *ctx, TplKeyword keyword)
{
    bool symbol_maybe = TRY(Bool, template_lexer_allow_keyword(ctx, keyword));
    if (!symbol_maybe) {
        ERROR(Bool, TemplateError, __LINE__, "Required keyword '%s'", TplKeyword_name(keyword));
    }
    RETURN(Bool, true);
}

ErrorOrOptionalTplOperatorMapping template_lexer_operator(TemplateParserContext *ctx)
{
    TplToken lookahead = TRY_TO(TplToken, OptionalTplOperatorMapping, template_lexer_next(ctx));
    switch (lookahead.type) {
    case TTTOperator:
        trace(CAT_TEMPLATE, "template_lexer_operator: %s", TplOpToken_name(lookahead.op));
        RETURN(OptionalTplOperatorMapping, template_operator_mapping(lookahead.op));
    default:
        trace(CAT_TEMPLATE, "template_lexer_operator: empty");
        RETURN(OptionalTplOperatorMapping,
            OptionalTplOperatorMapping_empty());
    }
}
