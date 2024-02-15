/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __LEXER_H__
#define __LEXER_H__

#include <ctype.h>

#include <sv.h>
#include <token.h>

#define NO_DIRECTIVE (-1)

typedef struct lexer Lexer;
typedef int          (*DirectiveHandler)(Lexer *lexer, int directive);

typedef struct {
    StringView       name;
    Keyword         *keywords;
    Token            preprocessor_trigger;
    char const     **directives;
    DirectiveHandler directive_handler;
    void            *language_data;
} Language;

typedef struct source {
    StringView     name;
    size_t         location;
    StringView     source;
    struct source *prev;
} Source;

typedef struct lexer {
    bool      whitespace_significant;
    bool      include_comments;
    Source   *sources;
    Token     current;
    Keyword  *keywords;
    Language *language;
    bool      in_comment;
    int       current_directive;
    void     *language_data;
} Lexer;

extern Lexer        lexer_create();
extern Lexer        lexer_for_language(Language *language);
extern StringView   lexer_source(Lexer *lexer);
extern size_t       lexer_current_location(Lexer *lexer);
extern Token        lexer_set_current(Lexer *lexer, Token token);
extern void         lexer_push_source(Lexer *lexer, StringView source, StringView name);
extern void         lexer_pop_source(Lexer *lexer);
extern void         lexer_advance_source(Lexer *lexer, size_t num);
extern Token        lexer_peek_next(Lexer *lexer);
extern Token        lexer_peek(Lexer *lexer);
extern Token        lexer_next(Lexer *lexer);
extern Token        lexer_lex(Lexer *lexer);
extern ErrorOrToken lexer_expect(Lexer *lexer, TokenKind kind, TokenCode code, char const *msg, ...);
extern bool         lexer_next_matches(Lexer *lexer, TokenKind kind, TokenCode code);

#define LEXER_LOC_ARG(lexer) LOC_ARG(lexer->sources->loc)

#endif /* __LEXER_H__ */
