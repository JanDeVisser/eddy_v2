/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __LANG_C_H__
#define __LANG_C_H__

#include <lexer.h>
#include <widget.h>

typedef struct {
    _W;
} CMode;

SIMPLE_WIDGET_CLASS(CMode, c_mode);

int handle_c_directive(Lexer *lexer, int directive);

typedef enum {
    CDirectiveElse,
    CDirectiveElif,
    CDirectiveElifdef,
    CDirectiveElifndef,
    CDirectiveEndif,
    CDirectiveError,
    CDirectiveDefine,
    CDirectiveIfdef,
    CDirectiveIfndef,
    CDirectiveIf,
    CDirectiveInclude,
    CDirectiveMax,
} CDirective;


static char const *c_directives[CDirectiveMax+1] = {
    [CDirectiveElse] = "else",
    [CDirectiveElif] = "elif",
    [CDirectiveElifdef] = "elifdef",
    [CDirectiveElifndef] = "elifndef",
    [CDirectiveEndif] = "endif",
    [CDirectiveError] = "error",
    [CDirectiveDefine] = "define",
    [CDirectiveIfdef] = "ifdef",
    [CDirectiveIfndef] = "ifndef",
    [CDirectiveIf] = "if",
    [CDirectiveInclude] = "include",
    [CDirectiveMax] = NULL,
};

#define KEYWORDS(S)   \
    S(alignas)        \
    S(alignof)        \
    S(auto)           \
    S(bool)           \
    S(break)          \
    S(case)           \
    S(char)           \
    S(const)          \
    S(constexpr)      \
    S(continue)       \
    S(default)        \
    S(do)             \
    S(double)         \
    S(else)           \
    S(enum)           \
    S(extern)         \
    S(false)          \
    S(float)          \
    S(for)            \
    S(goto)           \
    S(if)             \
    S(inline)         \
    S(int)            \
    S(long)           \
    S(nullptr)        \
    S(register)       \
    S(restrict)       \
    S(return)         \
    S(short)          \
    S(signed)         \
    S(sizeof)         \
    S(static)         \
    S(static_assert)  \
    S(struct)         \
    S(switch)         \
    S(thread_local)   \
    S(true)           \
    S(typedef)        \
    S(typeof)         \
    S(typeof_unqual)  \
    S(union)          \
    S(unsigned)       \
    S(void)           \
    S(volatile)       \
    S(while)          \
    S(_Alignas)       \
    S(_Alignof)       \
    S(_Atomic)        \
    S(_BitInt)        \
    S(_Bool)          \
    S(_Complex)       \
    S(_Decimal128)    \
    S(_Decimal32)     \
    S(_Decimal64)     \
    S(_Generic)       \
    S(_Imaginary)     \
    S(_Noreturn)      \
    S(_Static_assert) \
    S(_Thread_local)

static Keyword c_keywords[] = {
#undef KEYWORD_ENUM
#define KEYWORD_ENUM(kw) { .keyword = #kw, .code = __COUNTER__ },
    KEYWORDS(KEYWORD_ENUM)
#undef KEYWORD_ENUM
        { NULL, 0 },
};

static Language c_language = {
    .name = (StringView) { .ptr = "C", .length = 1 },
    .directives = c_directives,
    .preprocessor_trigger = (Token) { .code = '#', .kind = TK_SYMBOL },
    .keywords = c_keywords,
    .directive_handler = handle_c_directive,
};

#endif /* __LANG_C_H__ */
