/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdarg.h>

#include <base/error_or.h>
#include <base/io.h>
#include <base/lexer.h>

static int   isbdigit(int ch);
static Token scan_number(char const *buffer);

int isbdigit(int ch)
{
    return ch == '0' || ch == '1';
}

Token scan_number(char const *buffer)
{
    NumberType type = NTInteger;
    int        ix = 0;
    int (*predicate)(int) = isdigit;
    if (buffer[1] && buffer[0] == '0') {
        if (buffer[1] == 'x' || buffer[1] == 'X') {
            if (!buffer[2] || !isxdigit(buffer[2])) {
                return (Token) {
                    .kind = TK_NUMBER,
                    .text = { buffer, 1 },
                    .location = 0,
                    .number_type = type,
                };
            }
            type = NTHexNumber;
            predicate = isxdigit;
            ix = 2;
        } else if (buffer[1] == 'b' || buffer[1] == 'B') {
            if (!buffer[2] || !isxdigit(buffer[2])) {
                return (Token) {
                    .kind = TK_NUMBER,
                    .text = { buffer, 1 },
                    .location = 0,
                    .number_type = type,
                };
            }
            type = NTBinaryNumber;
            predicate = isbdigit;
            ix = 2;
        }
    }

    while (true) {
        char ch = buffer[ix];
        if (!predicate(ch) && ((ch != '.') || (type == NTDecimal))) {
            // FIXME lex '1..10' as '1', '..', '10'. It will now lex as '1.', '.', '10'
            return (Token) {
                .kind = TK_NUMBER,
                .text = { buffer, ix },
                .location = 0,
                .number_type = type,
            };
        }
        if (ch == '.') {
            if (type != NTInteger) {
                return (Token) {
                    .kind = TK_NUMBER,
                    .text = { buffer, ix },
                    .location = 0,
                    .number_type = type,
                };
            }
            type = NTDecimal;
        }
        ++ix;
    }
}

Lexer lexer_create()
{
    Lexer ret = { 0 };
    return ret;
}

Lexer lexer_for_language(Language *language)
{
    Lexer ret = { 0 };
    ret.language = language;
    return ret;
}

StringView lexer_source(Lexer *lexer)
{
    if (!lexer->sources) {
        return sv_null();
    }
    return lexer->sources->source;
}

StringView lexer_keyword(Lexer *lexer, int code)
{
    if (lexer->language && lexer->language->keywords && lexer->language->keywords[0].keyword) {
        for (int kw = 0; lexer->language->keywords[kw].code; ++kw) {
            if (lexer->language->keywords[kw].code == code) {
                return sv_from(lexer->language->keywords[kw].keyword);
            }
        }
    }
    return sv_null();
}

void lexer_push_source(Lexer *lexer, StringView source, StringView name)
{
    Source *entry = MALLOC(Source);
    entry->source = source;
    entry->location.file = name;
    entry->prev = lexer->sources;
    lexer->sources = entry;
    lexer->current = (Token) { 0 };
}

void lexer_pop_source(Lexer *lexer)
{
    if (lexer->sources) {
        lexer->sources = lexer->sources->prev;
    }
    lexer->current = (Token) { 0 };
}

Token directive_handle(Lexer *lexer, Token trigger)
{
    char const *buffer = lexer_source(lexer).ptr;
    size_t      directive_start = 1;

    assert(lexer->language != NULL);
    if (lexer->language->directives == NULL || lexer->language->directives[0] == NULL) {
        return trigger;
    }
    while (buffer[directive_start] && isspace(buffer[directive_start])) {
        ++directive_start;
    }
    if (!buffer[directive_start]) {
        return trigger;
    }
    size_t directive_end = directive_start;
    while (buffer[directive_end] && isalpha(buffer[directive_end]))
        ++directive_end;
    if (directive_end == directive_start) {
        return trigger;
    }
    StringView directive = { buffer + directive_start, directive_end - directive_start };
    for (int ix = 0; lexer->language->directives[ix]; ++ix) {
        if (sv_eq_cstr(directive, lexer->language->directives[ix])) {
            lexer->current_directive = ix + 1;
            return (Token) { .kind = TK_DIRECTIVE, .directive = ix, .text = { buffer, directive_end } };
        }
    }
    return trigger;
}

Token lexer_set_current(Lexer *lexer, Token token)
{
    if (lexer->sources) {
        token.location = lexer->sources->location;
    }
    lexer->current = token;
    return lexer->current;
}

Token lexer_peek(Lexer *lexer)
{
    if (lexer->current.kind != TK_UNKNOWN) {
        return lexer->current;
    }
    if (lexer->current_directive) {
        assert(lexer->language != NULL);
        lexer->current_directive = lexer->language->directive_handler(lexer, lexer->current_directive - 1) + 1;
        if (lexer->current_directive) {
            assert(lexer->current.kind != TK_UNKNOWN);
            return lexer->current;
        }
    }
    return lexer_set_current(lexer, lexer_peek_next(lexer));
}

Token block_comment(Lexer *lexer, char const *buffer, size_t ix)
{
    for (; buffer[ix] && buffer[ix] != '\n' && (buffer[ix - 1] != '*' || buffer[ix] != '/'); ++ix)
        ;
    if (!buffer[ix]) {
        return (Token) {
            .kind = TK_COMMENT,
            .comment = {
                .comment_type = CTBlock,
                .terminated = false,
            },
            .text = { buffer, ix },
        };
    }
    if (buffer[ix] == '\n') {
        lexer->in_comment = true;
        return (Token) {
            .kind = TK_COMMENT,
            .comment = {
                .comment_type = CTBlock,
                .terminated = true,
            },
            .text = { buffer, ix },
        };
    }
    lexer->in_comment = false;
    return (Token) {
        .kind = TK_COMMENT,
        .comment = {
            .comment_type = CTBlock,
            .terminated = true,
        },
        .text = { buffer, ix + 1 },
    };
}

Token lexer_peek_next(Lexer *lexer)
{
    if (lexer->current.kind != TK_UNKNOWN) {
        return lexer->current;
    }
    StringView  source = lexer_source(lexer);
    char const *buffer = source.ptr;
    if (!buffer || !buffer[0]) {
        return (Token) {
            .kind = TK_END_OF_FILE,
            .text = { buffer, 0 },
        };
    }
    if (lexer->in_comment) {
        if (buffer[0] == '\n') {
            return (Token) {
                .kind = TK_END_OF_LINE,
                .text = { buffer, 1 },
            };
        }
        return block_comment(lexer, buffer, 0);
    }
    switch (buffer[0]) {
    case '\'':
    case '"':
    case '`': {
        size_t ix = 1;
        while (buffer[ix] && buffer[ix] != buffer[0]) {
            if (buffer[ix] == '\\')
                ++ix;
            if (buffer[ix])
                ++ix;
        }
        return (Token) {
            .kind = TK_QUOTED_STRING,
            .quoted_string = {
                .quote_type = (QuoteType) buffer[0],
                .triple = false,
                .terminated = buffer[ix] != 0,
            },
            .text = { buffer, ix + 1 },
        };
    }
    case '/':
        switch (buffer[1]) {
        case '/': {
            size_t ix = 2;
            for (; buffer[ix] && buffer[ix] != '\n'; ++ix)
                ;
            return (Token) {
                .kind = TK_COMMENT,
                .comment = {
                    .comment_type = CTLine,
                    .terminated = true,
                },
                .text = { buffer, ix },
            };
        }
        case '*': {
            return block_comment(lexer, buffer, 2);
        }
        default:
            break;
        }
        break;
    default:
        break;
    }
    if (buffer[0] == '\n') {
        return (Token) {
            .kind = TK_END_OF_LINE,
            .text = { buffer, 1 },
        };
    }
    if (isspace(buffer[0])) {
        size_t ix = 0;
        for (; isspace(buffer[ix]) && buffer[ix] != '\n'; ++ix)
            ;
        return (Token) {
            .kind = TK_WHITESPACE,
            .text = { buffer, ix },
        };
    }
    if (isdigit(buffer[0])) {
        return scan_number(buffer);
    }
    if (isalpha(buffer[0]) || buffer[0] == '_') {
        size_t ix = 0;
        for (; isalnum(buffer[ix]) || buffer[ix] == '_'; ++ix)
            ;
        if (lexer->language && lexer->language->keywords && lexer->language->keywords[0].keyword) {
            for (int kw = 0; lexer->language->keywords[kw].keyword; ++kw) {
                StringView keyword = sv_from(lexer->language->keywords[kw].keyword);
                if (keyword.length == ix && sv_startswith(source, keyword)) {
                    return (Token) {
                        .kind = TK_KEYWORD,
                        .keyword_code = lexer->language->keywords[kw].code,
                        .text = { buffer, ix },
                    };
                }
            }
        }
        return (Token) {
            .kind = TK_IDENTIFIER,
            .text = { buffer, ix },
        };
    }
    if (lexer->language && lexer->language->keywords && lexer->language->keywords[0].keyword) {
        int        matched = -1;
        StringView matched_keyword = sv_null();
        for (int kw = 0; lexer->language->keywords[kw].code; ++kw) {
            StringView keyword = sv_from(lexer->language->keywords[kw].keyword);
            if (sv_startswith(source, keyword)) {
                if (matched < 0 || keyword.length > matched_keyword.length) {
                    matched = kw;
                    matched_keyword = keyword;
                }
            }
        }
        if (matched >= 0) {
            return (Token) {
                .kind = TK_KEYWORD,
                .keyword_code = lexer->language->keywords[matched].code,
                .text = { buffer, matched_keyword.length },
            };
        }
    }
    Token ret = (Token) {
        .kind = TK_SYMBOL,
        .symbol = (int) buffer[0],
        .text = { buffer, 1 },
    };
    if (lexer->language && lexer->language->preprocessor_trigger.symbol && !lexer->current_directive && lexer->language->preprocessor_trigger.symbol == ret.symbol) {
        return directive_handle(lexer, ret);
    }
    return ret;
}

Token lexer_next(Lexer *lexer)
{
    Token token = { 0 };
    while (lexer->sources) {
        for (token = lexer_peek(lexer); token.kind != TK_END_OF_FILE; token = lexer_peek(lexer)) {
            // clang-format off
            if ((lexer->whitespace_significant && (token.kind == TK_WHITESPACE || token.kind == TK_END_OF_LINE)) ||
                (lexer->include_comments && token.kind == TK_COMMENT) ||
                (token.kind != TK_WHITESPACE && token.kind != TK_COMMENT && token.kind != TK_END_OF_LINE)) {
                return token;
            }
            // clang-format on
            lexer_lex(lexer);
        }
        lexer_pop_source(lexer);
    }
    return token;
}

Token lexer_lex(Lexer *lexer)
{
    Token ret = lexer->current;
    if (ret.kind == TK_UNKNOWN) {
        ret = lexer_next(lexer);
    }
    if (lexer->sources) {
        Source *src = lexer->sources;
        lexer->sources->location.index += ret.text.length;
        lexer->sources->source = sv_lchop(lexer->sources->source, ret.text.length);
        if (ret.kind == TK_END_OF_LINE) {
            ++lexer->sources->location.line;
            lexer->sources->location.column = 0;
        } else {
            lexer->sources->location.column += ret.text.length;
        }
        if (!src->source.length) {
            src->source.ptr = NULL;
        }
    }
    lexer->current = (Token) { 0 };
    return ret;
}

ErrorOrToken lexer_expect(Lexer *lexer, TokenKind kind, char const *msg, ...)
{
    Token ret = lexer_next(lexer);
    if (!token_matches_kind(ret, kind)) {
        va_list args;
        va_start(args, msg);
        StringView formatted = sv_vprintf(msg, args);
        va_end(args);
        ERROR(Token, LexerError, 0, "%05zu:%.*s", lexer->sources->location, SV_ARG(formatted));
    }
    RETURN(Token, lexer_lex(lexer));
}

ErrorOrToken lexer_expect_symbol(Lexer *lexer, int symbol, char const *msg, ...)
{
    Token ret = lexer_next(lexer);
    if (ret.kind != TK_SYMBOL || ret.symbol != symbol) {
        va_list args;
        va_start(args, msg);
        StringView formatted = sv_vprintf(msg, args);
        va_end(args);
        ERROR(Token, LexerError, 0, "%05zu:%.*s", lexer->sources->location, SV_ARG(formatted));
    }
    RETURN(Token, lexer_lex(lexer));
}

ErrorOrToken lexer_expect_identifier(Lexer *lexer, char const *msg, ...)
{
    Token ret = lexer_next(lexer);
    if (ret.kind != TK_IDENTIFIER) {
        va_list args;
        va_start(args, msg);
        StringView formatted = sv_vprintf(msg, args);
        va_end(args);
        ERROR(Token, LexerError, 0, "%05zu:%.*s", lexer->sources->location, SV_ARG(formatted));
    }
    RETURN(Token, lexer_lex(lexer));
}

bool lexer_next_matches(Lexer *lexer, TokenKind kind)
{
    Token next = lexer_next(lexer);
    return token_matches_kind(next, kind);
}

bool lexer_next_matches_symbol(Lexer *lexer, int symbol)
{
    Token next = lexer_next(lexer);
    return next.kind == TK_SYMBOL && next.symbol == symbol;
}
