/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __PALETTE_H__
#define __PALETTE_H__

#include <stdint.h>

#define PALETTEINDICES(S)         \
    S(DEFAULT)                    \
    S(KEYWORD)                    \
    S(NUMBER)                     \
    S(STRING)                     \
    S(CHAR_LITERAL)               \
    S(PUNCTUATION)                \
    S(PREPROCESSOR)               \
    S(IDENTIFIER)                 \
    S(KNOWN_IDENTIFIER)           \
    S(PREPROC_IDENTIFIER)         \
    S(COMMENT)                    \
    S(MULTI_LINE_COMMENT)         \
    S(BACKGROUND)                 \
    S(CURSOR)                     \
    S(SELECTION)                  \
    S(ERROR_MARKER)               \
    S(BREAKPOINT)                 \
    S(LINE_NUMBER)                \
    S(CURRENT_LINE_FILL)          \
    S(CURRENT_LINE_FILL_INACTIVE) \
    S(CURRENT_LINE_EDGE)          \
    S(LINE_EDITED)                \
    S(LINE_EDITED_SAVED)          \
    S(LINE_EDITED_REVERTED)       \
    S(MAX)

typedef enum {
#undef PALETTEINDEX
#define PALETTEINDEX(I) PI_##I,
    PALETTEINDICES(PALETTEINDEX)
#undef PALETTEINDEX
} PaletteIndex;

#define PALETTES(S) \
    S(DARK)         \
    S(LIGHT)        \
    S(RETRO_BLUE)   \
    S(MAX)

typedef enum {
#undef PALETTE(P)
#define PALETTE(P) PALETTE_##P,
    PALETTES(PALETTE)
#undef PALETTE(P)
} Palette;

#define ANSICOLORS(S) \
    S(BLACK, 0xff000000)          \
    S(RED, 0xff0000cc)            \
    S(GREEN, 0xff069a4e)          \
    S(YELLOW, 0xff00a0c4)         \
    S(BLUE, 0xffcf9f72)           \
    S(MAGENTA, 0xff7b5075)        \
    S(CYAN, 0xff9a9806)           \
    S(WHITE, 0xffcfd7d3)          \
    S(BRIGHT_BLACK, 0xff535755)   \
    S(BRIGHT_RED, 0xff2929ef)     \
    S(BRIGHT_GREEN, 0xff34e28a)   \
    S(BRIGHT_YELLOW, 0xff4fe9fc)  \
    S(BRIGHT_BLUE, 0xffffaf32)    \
    S(BRIGHT_MAGENTA, 0xffa87fad) \
    S(BRIGHT_CYAN, 0xffe2e234)    \
    S(BRIGHT_WHITE, 0xffffffff)

typedef enum {
#undef ANSICOLOR
#define ANSICOLOR(C,RGB) ANSI_##C,
    ANSICOLORS(ANSICOLOR)
#undef ANSICOLOR
    ANSI_MAX,
} AnsiColor;

typedef uint32_t PaletteDefinition[PI_MAX];
typedef uint32_t PaletteDefinition[PI_MAX];

extern PaletteDefinition palettes[PALETTE_MAX];
extern uint32_t ansi_palette[ANSI_MAX];

#endif /* __PALETTE_H__ */
