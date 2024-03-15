/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <error_or.h>
#include <io.h>
#include <lexer.h>
#include <stdarg.h>
// #include <parser.h>

Token scan_number(char const *buffer)
{
    TokenCode code = TC_INTEGER;
    int       ix = 0;
    int (*predicate)(int) = isdigit;
    if (buffer[1] && buffer[0] == '0' && (buffer[1] == 'x' || buffer[1] == 'X')) {
        if (!buffer[2] || !isxdigit(buffer[2])) {
            return (Token) { TK_NUMBER, code, { buffer, 1 } };
        }
        code = TC_HEXNUMBER;
        predicate = isxdigit;
        ix = 2;
    }

    while (true) {
        char ch = buffer[ix];
        if (!predicate(ch) && ((ch != '.') || (code == TC_DECIMAL))) {
            // FIXME lex '1..10' as '1', '..', '10'. It will now lex as '1.', '.', '10'
            return (Token) { TK_NUMBER, code, { buffer, ix } };
        }
        if (ch == '.') {
            if (code == TC_HEXNUMBER) {
                return (Token) { TK_NUMBER, code, { buffer, ix } };
            }
            code = TC_DECIMAL;
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

size_t lexer_current_location(Lexer *lexer)
{
    size_t ret = 0;
    if (lexer->sources) {
        ret = lexer->sources->location;
    }
    return ret;
}

void lexer_advance_source(Lexer *lexer, size_t num)
{
    if (lexer->sources) {
        Source *src = lexer->sources;
        if (src->source.length < num) {
            num = src->source.length;
        }
        // for (size_t ix = 0; ix < num; ++ix) {
        //     if (src->source.ptr[ix] == '\n') {
        //         ++src->loc.line;
        //         src->loc.column = 1;
        //     } else {
        //         ++src->loc.column;
        //     }
        // }
        src->location += num;
        src->source.ptr += num;
        src->source.length -= num;
        if (!src->source.length) {
            src->source.ptr = NULL;
        }
    }
}

void lexer_push_source(Lexer *lexer, StringView source, StringView name)
{
    Source *entry = MALLOC(Source);
    entry->source = source;
    entry->name = name;
    entry->location = 0;
    entry->prev = lexer->sources;
    lexer->sources = entry;
    lexer->current.text = sv_null();
    lexer->current.kind = TK_UNKNOWN;
    lexer->current.code = TC_NONE;
}

void lexer_pop_source(Lexer *lexer)
{
    if (lexer->sources) {
        lexer->sources = lexer->sources->prev;
    }
    lexer->current.text = sv_null();
    lexer->current.kind = TK_UNKNOWN;
    lexer->current.code = TC_NONE;
}

Token directive_handle(Lexer *lexer, Token trigger)
{
    char const *buffer = lexer_source(lexer).ptr;
    size_t      directive_start = 1;
    size_t      text_start = 0;

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
            return (Token) { TK_DIRECTIVE, ix, { buffer, directive_end } };
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
        return (Token) { TK_COMMENT, TC_UNTERMINATED_BLOCK_COMMENT, { buffer, ix } };
    }
    if (buffer[ix] == '\n') {
        lexer->in_comment = true;
        return (Token) { TK_COMMENT, TC_BLOCK_COMMENT, { buffer, ix } };
    }
    lexer->in_comment = false;
    return (Token) { TK_COMMENT, TC_BLOCK_COMMENT, { buffer, ix + 1 } };
}

Token lexer_peek_next(Lexer *lexer)
{
    if (lexer->current.kind != TK_UNKNOWN) {
        return lexer->current;
    }
    StringView  source = lexer_source(lexer);
    char const *buffer = source.ptr;
    if (!buffer || !buffer[0]) {
        return (Token) { TK_END_OF_FILE, TC_NONE, { buffer, 0 } };
    }
    if (buffer[0] == '\n') {
        return (Token) { TK_WHITESPACE, TC_NEWLINE, { buffer, 1 } };
    }
    if (lexer->in_comment) {
        return block_comment(lexer, buffer, 0);
    }
    switch (buffer[0]) {
    case ' ':
    case '\t': {
        size_t ix = 0;
        for (; buffer[ix] == ' ' || buffer[ix] == '\t'; ++ix)
            ;
        return (Token) { TK_WHITESPACE, TC_WHITESPACE, { buffer, ix } };
    }
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
        TokenCode code;
        switch (buffer[0]) {
        case '"':
            code = TC_DOUBLE_QUOTED_STRING;
            break;
        case '\'':
            code = TC_SINGLE_QUOTED_STRING;
            break;
        case '`':
            code = TC_BACK_QUOTED_STRING;
            break;
        default:
            UNREACHABLE();
        }
        if (!buffer[ix]) {
            code += (TC_UNTERMINATED_DOUBLE_QUOTED_STRING - TC_DOUBLE_QUOTED_STRING);
        }
        return (Token) { TK_QUOTED_STRING, code, { buffer, ix + 1 } };
    }
    case '/':
        switch (buffer[1]) {
        case '/': {
            size_t ix = 2;
            for (; buffer[ix] && buffer[ix] != '\n'; ++ix)
                ;
            return (Token) { TK_COMMENT, TC_END_OF_LINE_COMMENT, { buffer, ix } };
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
    if (isspace(buffer[0])) {
        size_t ix = 0;
        for (; isspace(buffer[ix]) && buffer[ix] != '\n'; ++ix)
            ;
        return (Token) { TK_WHITESPACE, TC_WHITESPACE, { buffer, ix + 1 } };
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
                    return (Token) { TK_KEYWORD, lexer->language->keywords[kw].code, { buffer, ix } };
                }
            }
        }
        return (Token) { TK_IDENTIFIER, TC_IDENTIFIER, { buffer, ix } };
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
            return (Token) { TK_KEYWORD, TC_COUNT + matched, { buffer, matched_keyword.length } };
        }
    }
    Token ret = (Token) { TK_SYMBOL, (int) buffer[0], { buffer, 1 } };
    if (lexer->language && lexer->language->preprocessor_trigger.code && !lexer->current_directive && lexer->language->preprocessor_trigger.code == ret.code) {
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
            if ((lexer->whitespace_significant && token.kind == TK_WHITESPACE) ||
                (lexer->include_comments && token.kind == TK_COMMENT) ||
                (token.kind != TK_WHITESPACE && token.kind != TK_COMMENT)) {
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
    if (lexer->current.kind == TK_UNKNOWN) {
        lexer_next(lexer);
    }
    lexer_advance_source(lexer, lexer->current.text.length);
    Token ret = lexer->current;
    lexer->current.text.ptr = lexer_source(lexer).ptr;
    lexer->current.text.length = 0;
    lexer->current.kind = TK_UNKNOWN;
    lexer->current.code = TC_NONE;
    return ret;
}

ErrorOrToken lexer_expect(Lexer *lexer, TokenKind kind, TokenCode code, char const *msg, ...)
{
    Token ret = lexer_next(lexer);
    if (!token_matches(ret, kind, code)) {
        va_list args;
        va_start(args, msg);
        StringView formatted = sv_vprintf(msg, args);
        va_end(args);
        ERROR(Token, LexerError, 0, "%05zu:%.*s", lexer->sources->location, SV_ARG(formatted));
    }
    RETURN(Token, lexer_lex(lexer));
}

bool lexer_next_matches(Lexer *lexer, TokenKind kind, TokenCode code)
{
    Token next = lexer_next(lexer);
    return token_matches(next, kind, code);
}
