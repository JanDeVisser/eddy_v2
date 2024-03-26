/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <base/log.h>
#include <palette.h>

PaletteDefinition palettes[PALETTE_MAX] = {
    {
        // Dark palette
        [PI_DEFAULT] = (Color) { .a = 0xff, .b = 0xff, .g = 0xff, .r = 0xff },                    // None.
        [PI_KEYWORD] = (Color) { .a = 0xff, .b = 0xd6, .g = 0x9c, .r = 0x56 },                    // Keyword.
        [PI_NUMBER] = (Color) { .a = 0xff, .b = 0xa8, .g = 0xce, .r = 0xb5 },                     // Number.
        [PI_STRING] = (Color) { .a = 0xff, .b = 0x85, .g = 0x9d, .r = 0xd6 },                     // String.
        [PI_CHAR_LITERAL] = (Color) { .a = 0xff, .b = 0x70, .g = 0xa0, .r = 0xe0 },               // Char literal.
        [PI_PUNCTUATION] = (Color) { .a = 0xff, .b = 0xb4, .g = 0xb4, .r = 0xb4 },                // Punctuation.
        [PI_PREPROCESSOR] = (Color) { .a = 0xff, .b = 0x3b, .g = 0x77, .r = 0xbe },               // Preprocessor directive.
        [PI_PREPROCESSOR_ARG] = (Color) { .a = 0xff, .b = 0x6d, .g = 0x9c, .r = 0x7b },           // Preprocessor directive argument.
        [PI_PREPROCESSOR_DEFINE] = (Color) { .a = 0xff, .b = 0x69, .g = 0xaf, .r = 0xdb },        // Preprocessor directive argument.
        [PI_IDENTIFIER] = (Color) { .a = 0xff, .b = 0xda, .g = 0xda, .r = 0xda },                 // Identifier.
        [PI_KNOWN_IDENTIFIER] = (Color) { .a = 0xff, .b = 0xb0, .g = 0xc9, .r = 0x4e },           // Known identifier.
        [PI_VARIABLE] = (Color) { .a = 0xff, .b = 0xc6, .g = 0xb7, .r = 0xa9 },                   // Known identifier.
        [PI_PROPERTY] = (Color) { .a = 0xff, .b = 0xbe, .g = 0x72, .r = 0xa2 },                   // Known identifier.
        [PI_FUNCTION] = (Color) { .a = 0xff, .b = 0x60, .g = 0xc6, .r = 0xff },                   // Comment (single line).
        [PI_COMMENT] = (Color) { .a = 0xff, .b = 0x80, .g = 0x80, .r = 0x80 },                    // Comment (single line).
        [PI_BACKGROUND] = (Color) { .a = 0xff, .b = 0x2c, .g = 0x2c, .r = 0x2c },                 // Background.
        [PI_CURSOR] = (Color) { .a = 0xff, .b = 0xe0, .g = 0xe0, .r = 0xe0 },                     // Cursor.
        [PI_SELECTION] = (Color) { .a = 0xff, .b = 0xa0, .g = 0x60, .r = 0x20 },                  // Selection.
        [PI_ERROR_MARKER] = (Color) { .a = 0x80, .b = 0x4d, .g = 0x00, .r = 0xff },               // ErrorMarker.
        [PI_BREAKPOINT] = (Color) { .a = 0x40, .b = 0xf0, .g = 0x80, .r = 0x00 },                 // Breakpoint.
        [PI_LINE_NUMBER] = (Color) { .a = 0xff, .b = 0xaf, .g = 0x91, .r = 0x2b },                // Line number.
        [PI_CURRENT_LINE_FILL] = (Color) { .a = 0x40, .b = 0x00, .g = 0x00, .r = 0x00 },          // Current line fill.
        [PI_CURRENT_LINE_FILL_INACTIVE] = (Color) { .a = 0x40, .b = 0x80, .g = 0x80, .r = 0x80 }, // Current line fill (inactive).
        [PI_CURRENT_LINE_EDGE] = (Color) { .a = 0x40, .b = 0xa0, .g = 0xa0, .r = 0xa0 },          // Current line edge.
        [PI_LINE_EDITED] = (Color) { .a = 0xff, .b = 0x84, .g = 0xf2, .r = 0xef },                // Line edited.
        [PI_LINE_EDITED_SAVED] = (Color) { .a = 0xff, .b = 0x30, .g = 0x74, .r = 0x57 },          // Line edited saved.
        [PI_LINE_EDITED_REVERTED] = (Color) { .a = 0xff, .b = 0xfa, .g = 0x95, .r = 0x5f },       // Line edited reverted.
    },
#if 0
    {
        // Light Palette
        [PI_DEFAULT] = (Color) { .a = 0xff, .b = 0x00, .g = 0x00, .r = 0x00 },                    // None.
        [PI_KEYWORD] = (Color) { .a = 0xff, .b = 0xff, .g = 0x0c, .r = 0x06 },                    // Keyword.
        [PI_NUMBER] = (Color) { .a = 0xff, .b = 0x00, .g = 0x80, .r = 0x00 },                     // Number.
        [PI_STRING] = (Color) { .a = 0xff, .b = 0x20, .g = 0x20, .r = 0xa0 },                     // String.
        [PI_CHAR_LITERAL] = (Color) { .a = 0xff, .b = 0x30, .g = 0x40, .r = 0x70 },               // Char literal.
        [PI_PUNCTUATION] = (Color) { .a = 0xff, .b = 0x00, .g = 0x00, .r = 0x00 },                // Punctuation.
        [PI_PREPROCESSOR] = (Color) { .a = 0xff, .b = 0x40, .g = 0x90, .r = 0x90 },               // Preprocessor directive
        [PI_PREPROCESSOR_ARG] = (Color) { .a = 0xff, .b = 0x40, .g = 0x90, .r = 0x90 },           // Preprocessor directive argument
        [PI_IDENTIFIER] = (Color) { .a = 0xff, .b = 0x40, .g = 0x40, .r = 0x40 },                 // Identifier.
        [PI_KNOWN_IDENTIFIER] = (Color) { .a = 0xff, .b = 0x60, .g = 0x60, .r = 0x10 },           // Known identifier.
        [PI_PREPROCESSOR_DEFINE] = (Color) { .a = 0xff, .b = 0xc0, .g = 0x40, .r = 0xa0 },        // Preproc identifier.
        [PI_FUNCTION] = (Color) { .a = 0xff, .b = 0x40, .g = 0x40, .r = 0x40 },                   // Identifier.
        [PI_COMMENT] = (Color) { .a = 0xff, .b = 0x20, .g = 0x50, .r = 0x20 },                    // Comment (single line).
        [PI_MULTI_LINE_COMMENT] = (Color) { .a = 0xff, .b = 0x40, .g = 0x50, .r = 0x20 },         // Comment (multi line).
        [PI_BACKGROUND] = (Color) { .a = 0xff, .b = 0xff, .g = 0xff, .r = 0xff },                 // Background.
        [PI_CURSOR] = (Color) { .a = 0xff, .b = 0x00, .g = 0x00, .r = 0x00 },                     // Cursor.
        [PI_SELECTION] = (Color) { .a = 0xff, .b = 0x60, .g = 0x00, .r = 0x00 },                  // Selection.
        [PI_ERROR_MARKER] = (Color) { .a = 0xa0, .b = 0x00, .g = 0x10, .r = 0xff },               // ErrorMarker.
        [PI_BREAKPOINT] = (Color) { .a = 0x80, .b = 0xf0, .g = 0x80, .r = 0x00 },                 // Breakpoint.
        [PI_LINE_NUMBER] = (Color) { .a = 0xff, .b = 0x50, .g = 0x50, .r = 0x00 },                // Line number.
        [PI_CURRENT_LINE_FILL] = (Color) { .a = 0x40, .b = 0x00, .g = 0x00, .r = 0x00 },          // Current line fill.
        [PI_CURRENT_LINE_FILL_INACTIVE] = (Color) { .a = 0x40, .b = 0x80, .g = 0x80, .r = 0x80 }, // Current line fill (inactive).
        [PI_CURRENT_LINE_EDGE] = (Color) { .a = 0x40, .b = 0x00, .g = 0x00, .r = 0x00 },          // Current line edge.
        [PI_LINE_EDITED] = (Color) { .a = 0xff, .b = 0x84, .g = 0xf2, .r = 0xef },                // Line edited.
        [PI_LINE_EDITED_SAVED] = (Color) { .a = 0xff, .b = 0x30, .g = 0x74, .r = 0x57 },          // Line edited saved.
        [PI_LINE_EDITED_REVERTED] = (Color) { .a = 0xff, .b = 0xfa, .g = 0x95, .r = 0x5f },       // Line edited reverted.
    },
    {
        // Retro blue palette
        [PI_DEFAULT] = (Color) { .a = 0xff, .b = 0x00, .g = 0xff, .r = 0xff },                    // None.
        [PI_KEYWORD] = (Color) { .a = 0xff, .b = 0xff, .g = 0xff, .r = 0x00 },                    // Keyword.
        [PI_NUMBER] = (Color) { .a = 0xff, .b = 0x00, .g = 0xff, .r = 0x00 },                     // Number.
        [PI_STRING] = (Color) { .a = 0xff, .b = 0x80, .g = 0x80, .r = 0x00 },                     // String.
        [PI_CHAR_LITERAL] = (Color) { .a = 0xff, .b = 0x80, .g = 0x80, .r = 0x00 },               // Char literal.
        [PI_PUNCTUATION] = (Color) { .a = 0xff, .b = 0xff, .g = 0xff, .r = 0xff },                // Punctuation.
        [PI_PREPROCESSOR] = (Color) { .a = 0xff, .b = 0x00, .g = 0x80, .r = 0x00 },               // Preprocessor.
        [PI_PREPROCESSOR_ARG] = (Color) { .a = 0xff, .b = 0x00, .g = 0x80, .r = 0x00 },           // Preprocessor argument.
        [PI_IDENTIFIER] = (Color) { .a = 0xff, .b = 0x00, .g = 0xff, .r = 0xff },                 // Identifier.
        [PI_FUNCTION] = (Color) { .a = 0xff, .b = 0x00, .g = 0xff, .r = 0xff },                   // Identifier.
        [PI_KNOWN_IDENTIFIER] = (Color) { .a = 0xff, .b = 0xff, .g = 0xff, .r = 0xff },           // Known identifier.
        [PI_PREPROCESSOR_DEFINE] = (Color) { .a = 0xff, .b = 0xff, .g = 0x00, .r = 0xff },        // Preproc identifier.
        [PI_COMMENT] = (Color) { .a = 0xff, .b = 0x80, .g = 0x80, .r = 0x80 },                    // Comment (single line).
        [PI_MULTI_LINE_COMMENT] = (Color) { .a = 0xff, .b = 0x40, .g = 0x40, .r = 0x40 },         // Comment (multi line).
        [PI_BACKGROUND] = (Color) { .a = 0xff, .b = 0x80, .g = 0x00, .r = 0x00 },                 // Background.
        [PI_CURSOR] = (Color) { .a = 0xff, .b = 0x00, .g = 0x80, .r = 0xff },                     // Cursor.
        [PI_SELECTION] = (Color) { .a = 0xff, .b = 0xff, .g = 0xff, .r = 0x00 },                  // Selection.
        [PI_ERROR_MARKER] = (Color) { .a = 0xa0, .b = 0x00, .g = 0x00, .r = 0xff },               // ErrorMarker.
        [PI_BREAKPOINT] = (Color) { .a = 0x80, .b = 0xff, .g = 0x80, .r = 0x00 },                 // Breakpoint.
        [PI_LINE_NUMBER] = (Color) { .a = 0xff, .b = 0x80, .g = 0x80, .r = 0x00 },                // Line number.
        [PI_CURRENT_LINE_FILL] = (Color) { .a = 0x40, .b = 0x00, .g = 0x00, .r = 0x00 },          // Current line fill.
        [PI_CURRENT_LINE_FILL_INACTIVE] = (Color) { .a = 0x40, .b = 0x80, .g = 0x80, .r = 0x80 }, // Current line fill (inactive).
        [PI_CURRENT_LINE_EDGE] = (Color) { .a = 0x40, .b = 0x00, .g = 0x00, .r = 0x00 },          // Current line edge.
        [PI_LINE_EDITED] = (Color) { .a = 0xff, .b = 0x84, .g = 0xf2, .r = 0xef },                // Line edited.
        [PI_LINE_EDITED_SAVED] = (Color) { .a = 0xff, .b = 0x30, .g = 0x74, .r = 0x57 },          // Line edited saved.
        [PI_LINE_EDITED_REVERTED] = (Color) { .a = 0xff, .b = 0xfa, .g = 0x95, .r = 0x5f },       // Line edited reverted.
    },
#endif
};

uint32_t ansi_palette[ANSI_MAX] = {
#undef ANSICOLOR
#define ANSICOLOR(C, RGB) [ANSI_##C] = RGB,
    ANSICOLORS(ANSICOLOR)
#undef ANSICOLOR
};

char const *PaletteIndex_name(PaletteIndex index)
{
    switch (index) {
#undef PALETTEINDEX
#define PALETTEINDEX(I) \
    case PI_##I:        \
        return #I;
        PALETTEINDICES(PALETTEINDEX)
#undef PALETTEINDEX
    default:
        UNREACHABLE();
    }
}
