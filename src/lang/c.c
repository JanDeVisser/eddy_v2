/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <lang/c.h>

typedef enum {
    CDirectiveStateInit = 0,
    CDirectiveStateIncludeQuote,
    CDirectiveStateMacroName,
} CDirectiveState;

int handle_include_directive(Lexer *lexer)
{
    char const *buffer = lexer_source(lexer).ptr;
    size_t      ix = 0;
    size_t      state = (size_t) lexer->language_data;
    switch (state) {
    case CDirectiveStateInit: {
        while (buffer[ix] == ' ' || buffer[ix] == '\t') {
            ++ix;
        }
        lexer->language_data = (void *) CDirectiveStateIncludeQuote;
        if (ix > 0) {
            lexer_set_current(lexer, (Token) { .kind = TK_WHITESPACE, TC_WHITESPACE, { buffer, ix } });
            return CDirectiveInclude;
        }
        lexer->language_data = (void *) CDirectiveStateIncludeQuote;
    } // Fall through
    case CDirectiveStateIncludeQuote: {
        if (buffer[0] != '<' && buffer[0] != '"') {
            lexer->language_data = NULL;
            return NO_DIRECTIVE;
        }
        char end = (buffer[0] == '<') ? '>' : '"';
        while (buffer[ix] && buffer[ix] != end && buffer[ix] != '\n') {
            ++ix;
        }
        if (buffer[ix] == end) {
            lexer_set_current(lexer, (Token) { .kind = TK_DIRECTIVE_ARG, end, { buffer, ix + 1} });
        } else {
            lexer_set_current(lexer, (Token) { .kind = TK_DIRECTIVE_ARG, end, { buffer, ix } });
        }
    } break;
    default:
        UNREACHABLE();
    }
    lexer->language_data = NULL;
    return NO_DIRECTIVE;
}

int handle_macro_name_directive(Lexer *lexer)
{
    char const *buffer = lexer_source(lexer).ptr;
    size_t      ix = 0;
    size_t      state = (size_t) lexer->language_data;
    switch (state) {
    case CDirectiveStateInit: {
        while (buffer[ix] == ' ' || buffer[ix] == '\t') {
            ++ix;
        }
        lexer->language_data = (void *) CDirectiveStateMacroName;
        if (ix > 0) {
            lexer_set_current(lexer, (Token) { .kind = TK_WHITESPACE, TC_WHITESPACE, { buffer, ix } });
            return lexer->current_directive - 1;
        }
    }  // Fall through
    case CDirectiveStateMacroName: {
        Token t = lexer_peek_next(lexer);
        if (t.kind == TK_IDENTIFIER) {
            t.kind = TK_DIRECTIVE_ARG;
        }
        lexer_set_current(lexer, t);
    } break;
    default:
        UNREACHABLE();
    }
    lexer->language_data = (void *) NULL;
    return NO_DIRECTIVE;
}

int handle_c_directive(Lexer *lexer, int directive)
{
    switch (directive) {
    case CDirectiveInclude:
        return handle_include_directive(lexer);
    case CDirectiveDefine:
    case CDirectiveIfdef:
    case CDirectiveIfndef:
    case CDirectiveElifdef:
    case CDirectiveElifndef:
        return handle_macro_name_directive(lexer);
    case CDirectiveElse:
    case CDirectiveEndif:
    default:
        lexer->language_data = (void *) NULL;
        return NO_DIRECTIVE;
    }
}
