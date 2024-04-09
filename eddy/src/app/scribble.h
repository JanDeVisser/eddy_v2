/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef APP_SCRIBBLE_H
#define APP_SCRIBBLE_H

#include <app/mode.h>

typedef struct {
    _M;
} ScribbleMode;

MODE_CLASS(ScribbleMode, scribble_mode);

int handle_scribble_directive(Lexer *lexer, int directive);

typedef enum {
    ScribbleDirectiveInclude,
    ScribbleDirectiveMax,
} ScribbleDirective;

static char const *scribble_directives[ScribbleDirectiveMax + 1] = {
    [ScribbleDirectiveInclude] = "include",
    [ScribbleDirectiveMax] = NULL,
};

#define SCRIBBLE_KEYWORDS(S) \
    S(bool)                  \
    S(break)                 \
    S(case)                  \
    S(char)                  \
    S(const)                 \
    S(continue)              \
    S(do)                    \
    S(else)                  \
    S(enum)                  \
    S(false)                 \
    S(float)                 \
    S(for)                   \
    S(if)                    \
    S(int)                   \
    S(optional)              \
    S(return)                \
    S(struct)                \
    S(switch)                \
    S(true)                  \
    S(type)                  \
    S(variant)               \
    S(unsigned)              \
    S(void)                  \
    S(while)

static Keyword scribble_keywords[] = {
#undef S
#define S(kw) { .keyword = #kw, .code = __COUNTER__ },
    SCRIBBLE_KEYWORDS(S)
#undef S
        { NULL, 0 },
};

static Language scribble_language = {
    .name = SV("Scribble"),
    .directives = scribble_directives,
    .preprocessor_trigger = (Token) { .symbol = '$', .kind = TK_SYMBOL },
    .keywords = scribble_keywords,
    .directive_handler = handle_scribble_directive,
};

#endif /* APP_SCRIBBLE_H */
