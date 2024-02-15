/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __TOKEN_H__
#define __TOKEN_H__

#include <json.h>
#include <sv.h>

#define TOKENKINDS(S) \
    S(UNKNOWN)        \
    S(END_OF_FILE)    \
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
#undef TOKENKIND_ENUM
#define TOKENKIND_ENUM(kind) TK_##kind,
    TOKENKINDS(TOKENKIND_ENUM)
#undef TOKENKIND_ENUM
} TokenKind;

#define TOKENCODES(S)                       \
    S(TC_NONE)                              \
    S(TC_IDENTIFIER)                        \
    S(TC_INTEGER)                           \
    S(TC_DECIMAL)                           \
    S(TC_HEXNUMBER)                         \
    S(TC_BINARYNUMBER)                      \
    S(TC_SINGLE_QUOTED_STRING)              \
    S(TC_DOUBLE_QUOTED_STRING)              \
    S(TC_BACK_QUOTED_STRING)                \
    S(TC_UNTERMINATED_SINGLE_QUOTED_STRING) \
    S(TC_UNTERMINATED_DOUBLE_QUOTED_STRING) \
    S(TC_UNTERMINATED_BACK_QUOTED_STRING)   \
    S(TC_END_OF_LINE_COMMENT)               \
    S(TC_BLOCK_COMMENT)                     \
    S(TC_UNTERMINATED_BLOCK_COMMENT)        \
    S(TC_NEWLINE)                           \
    S(TC_WHITESPACE)

typedef enum {
#undef TOKENCODE_ENUM
#define TOKENCODE_ENUM(code) code,
    TOKENCODES(TOKENCODE_ENUM)
#undef TOKENCODE_ENUM
        TC_COUNT,
} TokenCode;

typedef struct {
    char const *keyword;
    int         code;
} Keyword;

typedef struct {
    TokenKind  kind;
    int        code;
    StringView text;
    size_t     location;
} Token;

ERROR_OR_ALIAS(Token, Token)
OPTIONAL(Token)

extern char const *TokenKind_name(TokenKind kind);
extern TokenKind   TokenKind_from_string(StringView kind);
extern char const *TokenCode_name(int code);
extern JSONValue   token_to_json(Token token);
extern Token       token_from_json(JSONValue token);
extern Token       token_merge(Token t1, Token t2, TokenKind kind, int code);

#define token_matches_kind(t, k) ((t).kind == k)
#define token_matches(t, k, c) (token_matches_kind((t), (k)) && (t).code == c)

#define TOKEN_LOC_ARG(token) LOC_ARG(token.loc)

#endif /* __TOKEN_H__ */
