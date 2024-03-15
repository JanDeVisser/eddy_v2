/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __LANG_SCRIBBLE_H__
#define __LANG_SCRIBBLE_H__

#include <base/lexer.h>

typedef enum {
    ScribbleDirectiveInclude,
    ScribbleDirectiveMax,
} ScribbleDirective;

static char const *scribble_directives[ScribbleDirectiveMax + 1] = {
    [ScribbleDirectiveInclude] = "include",
    [ScribbleDirectiveMax] = NULL
};

#define KEYWORDS(S)                \
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
    S(MACRO_BINDING, = >, 42)      \
    S(UNARY_DECREMENT, --, 43)     \
    S(UNARY_INCREMENT, ++, 44)

typedef enum {
#undef KEYWORD_ENUM
#define KEYWORD_ENUM(keyword, text, code) KW_##keyword = TC_COUNT + code,
    KEYWORDS(KEYWORD_ENUM)
#undef KEYWORD_ENUM
        KW_MAX,
} KeywordCode;

#define KW_COUNT (KW_MAX - TC_COUNT)

static Keyword scribble_keywords[] = {
#undef KEYWORD_ENUM
#define KEYWORD_ENUM(kw, text, c) { .keyword = #text, .code = c },
    KEYWORDS(KEYWORD_ENUM)
#undef KEYWORD_ENUM
        { NULL, 0 },
};

extern int scribble_handle_directive(Lexer *lexer, int directive);

static Language language_scribble = {
    .name = (StringView) { .ptr = "Scribble", .length = 7 },
    .directives = scribble_directives,
    .directive_handler = scribble_handle_directive,
    .preprocessor_trigger = (Token) { .code = '$', .kind = TK_SYMBOL },
    .keywords = scribble_keywords,
};

#endif /* __LANG_SCRIBBLE_H__ */
