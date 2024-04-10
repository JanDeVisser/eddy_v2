/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BASE_TOKEN_H
#define BASE_TOKEN_H

#include <base/json.h>
#include <base/sv.h>

#define TOKENKINDS(S) \
    S(UNKNOWN)        \
    S(END_OF_FILE)    \
    S(END_OF_LINE)    \
    S(SYMBOL)         \
    S(KEYWORD)        \
    S(IDENTIFIER)     \
    S(NUMBER)         \
    S(QUOTED_STRING)  \
    S(COMMENT)        \
    S(WHITESPACE)     \
    S(PROGRAM)        \
    S(DIRECTIVE)      \
    S(DIRECTIVE_ARG)  \
    S(MODULE)

typedef enum {
#undef S
#define S(kind) TK_##kind,
    TOKENKINDS(S)
#undef S
} TokenKind;

OPTIONAL(TokenKind)
ERROR_OR(TokenKind)

#define QUOTETYPES(S)    \
    S(SingleQuote, '\'') \
    S(DoubleQuote, '"')  \
    S(BackQuote, '`')

typedef enum {
#undef S
#define S(T, Q) QT##T = Q,
    QUOTETYPES(S)
#undef S
} QuoteType;

OPTIONAL(QuoteType)
ERROR_OR(QuoteType)

#define COMMENTTYPES(S) \
    S(Block)            \
    S(Line)

typedef enum {
#undef S
#define S(T) CT##T,
    COMMENTTYPES(S)
#undef S
} CommentType;

OPTIONAL(CommentType)
ERROR_OR(CommentType)

#define NUMBERTYPES(S) \
    S(Integer)         \
    S(Decimal)         \
    S(HexNumber)       \
    S(BinaryNumber)

typedef enum {
#undef S
#define S(T) NT##T,
    NUMBERTYPES(S)
#undef S
} NumberType;

OPTIONAL(NumberType)
ERROR_OR(NumberType)

typedef struct _location {
    StringView file;
    size_t     index;
    size_t     line;
    size_t     column;
} TokenLocation;

OPTIONAL(TokenLocation)
ERROR_OR(TokenLocation)

typedef struct {
    char const *keyword;
    int         code;
} Keyword;

typedef struct {
    TokenKind     kind;
    StringView    text;
    TokenLocation location;
    union {
        NumberType number_type;
        struct {
            QuoteType quote_type;
            bool      triple;
            bool      terminated;
        } quoted_string;
        struct {
            CommentType comment_type;
            bool        terminated;
        } comment;
        int keyword_code;
        int directive;
        int symbol;
    };
} Token;

ERROR_OR(Token)
OPTIONAL(Token)

extern StringView           TokenKind_name(TokenKind kind);
extern ErrorOrTokenKind     TokenKind_from_string(StringView kind);
extern StringView           QuoteType_name(QuoteType quote);
extern ErrorOrQuoteType     QuoteType_from_string(StringView quote);
extern StringView           CommentType_name(CommentType quote);
extern ErrorOrCommentType   CommentType_from_string(StringView comment);
extern StringView           NumberType_name(NumberType quote);
extern ErrorOrNumberType    NumberType_from_string(StringView comment);
extern JSONValue            location_to_json(TokenLocation location);
extern ErrorOrTokenLocation location_from_json(JSONValue location);
extern JSONValue            token_to_json(Token token);
extern ErrorOrToken         token_from_json(JSONValue token);

#define token_matches_kind(t, k) ((t).kind == k)
#define token_matches_symbol(t, s) (token_matches_kind((t), TK_SYMBOL) && (t).symbol == (s))
#define token_matches_keyword(t, c) (token_matches_kind((t), TK_KEYWORD) && (t).keyword_code == (c))
#define token_matches_identifier(t) (token_matches_kind((t), TK_IDENTIFIER))

#endif /* BASE_TOKEN_H */
