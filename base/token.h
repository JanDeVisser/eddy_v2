/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BASE_TOKEN_H
#define BASE_TOKEN_H

#include <base/json.h>
#include <base/sv.h>

/* THIS IS HERE FOR SAFEKEEPING */
// clang-format off
#define XX_KEYWORDS(S)             \
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

#endif /* BASE_TOKEN_H */
