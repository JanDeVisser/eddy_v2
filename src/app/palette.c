/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <palette.h>

PaletteDefinition palettes[PALETTE_MAX] = {
    {
        // Dark palette
        [PI_DEFAULT] = 0xffffffff,                    // None.
        [PI_KEYWORD] = 0xffd69c56,                    // Keyword.
        [PI_NUMBER] = 0xffa8ceb5,                     // Number.
        [PI_STRING] = 0xff859dd6,                     // String.
        [PI_CHAR_LITERAL] = 0xff70a0e0,               // Char literal.
        [PI_PUNCTUATION] = 0xffb4b4b4,                // Punctuation.
        [PI_PREPROCESSOR] = 0xff409090,               // Preprocessor.
        [PI_IDENTIFIER] = 0xffdadada,                 // Identifier.
        [PI_KNOWN_IDENTIFIER] = 0xffb0c94e,           // Known identifier.
        [PI_PREPROC_IDENTIFIER] = 0xffc040a0,         // Preproc identifier.
        [PI_COMMENT] = 0xff4aa657,                    // Comment (single line).
        [PI_MULTI_LINE_COMMENT] = 0xff4aa657,         // Comment (multi line).
        [PI_BACKGROUND] = 0xff2C2C2C,                 // Background.
        [PI_CURSOR] = 0xffe0e0e0,                     // Cursor.
        [PI_SELECTION] = 0xffa06020,                  // Selection.
        [PI_ERROR_MARKER] = 0x804d00ff,               // ErrorMarker.
        [PI_BREAKPOINT] = 0x40f08000,                 // Breakpoint.
        [PI_LINE_NUMBER] = 0xffaf912b,                // Line number.
        [PI_CURRENT_LINE_FILL] = 0x40000000,          // Current line fill.
        [PI_CURRENT_LINE_FILL_INACTIVE] = 0x40808080, // Current line fill (inactive).
        [PI_CURRENT_LINE_EDGE] = 0x40a0a0a0,          // Current line edge.
        [PI_LINE_EDITED] = 0xff84f2ef,                // Line edited.
        [PI_LINE_EDITED_SAVED] = 0xff307457,          // Line edited saved.
        [PI_LINE_EDITED_REVERTED] = 0xfffa955f,       // Line edited reverted.
    },
    { // Light Palette
        [PI_DEFAULT] = 0xff000000,                    // None.
        [PI_KEYWORD] = 0xffff0c06,                    // Keyword.
        [PI_NUMBER] = 0xff008000,                     // Number.
        [PI_STRING] = 0xff2020a0,                     // String.
        [PI_CHAR_LITERAL] = 0xff304070,               // Char literal.
        [PI_PUNCTUATION] = 0xff000000,                // Punctuation.
        [PI_PREPROCESSOR] = 0xff409090,               // Preprocessor.
        [PI_IDENTIFIER] = 0xff404040,                 // Identifier.
        [PI_KNOWN_IDENTIFIER] = 0xff606010,           // Known identifier.
        [PI_PREPROC_IDENTIFIER] = 0xffc040a0,         // Preproc identifier.
        [PI_COMMENT] = 0xff205020,                    // Comment (single line).
        [PI_MULTI_LINE_COMMENT] = 0xff405020,         // Comment (multi line).
        [PI_BACKGROUND] = 0xffffffff,                 // Background.
        [PI_CURSOR] = 0xff000000,                     // Cursor.
        [PI_SELECTION] = 0xff600000,                  // Selection.
        [PI_ERROR_MARKER] = 0xa00010ff,               // ErrorMarker.
        [PI_BREAKPOINT] = 0x80f08000,                 // Breakpoint.
        [PI_LINE_NUMBER] = 0xff505000,                // Line number.
        [PI_CURRENT_LINE_FILL] = 0x40000000,          // Current line fill.
        [PI_CURRENT_LINE_FILL_INACTIVE] = 0x40808080, // Current line fill (inactive).
        [PI_CURRENT_LINE_EDGE] = 0x40000000,          // Current line edge.
        [PI_LINE_EDITED] = 0xff84f2ef,                // Line edited.
        [PI_LINE_EDITED_SAVED] = 0xff307457,          // Line edited saved.
        [PI_LINE_EDITED_REVERTED] = 0xfffa955f,       // Line edited reverted.
    },
    { // Retro blue palette
        [PI_DEFAULT] = 0xff00ffff,                    // None.
        [PI_KEYWORD] = 0xffffff00,                    // Keyword.
        [PI_NUMBER] = 0xff00ff00,                     // Number.
        [PI_STRING] = 0xff808000,                     // String.
        [PI_CHAR_LITERAL] = 0xff808000,               // Char literal.
        [PI_PUNCTUATION] = 0xffffffff,                // Punctuation.
        [PI_PREPROCESSOR] = 0xff008000,               // Preprocessor.
        [PI_IDENTIFIER] = 0xff00ffff,                 // Identifier.
        [PI_KNOWN_IDENTIFIER] = 0xffffffff,           // Known identifier.
        [PI_PREPROC_IDENTIFIER] = 0xffff00ff,         // Preproc identifier.
        [PI_COMMENT] = 0xff808080,                    // Comment (single line).
        [PI_MULTI_LINE_COMMENT] = 0xff404040,         // Comment (multi line).
        [PI_BACKGROUND] = 0xff800000,                 // Background.
        [PI_CURSOR] = 0xff0080ff,                     // Cursor.
        [PI_SELECTION] = 0xffffff00,                  // Selection.
        [PI_ERROR_MARKER] = 0xa00000ff,               // ErrorMarker.
        [PI_BREAKPOINT] = 0x80ff8000,                 // Breakpoint.
        [PI_LINE_NUMBER] = 0xff808000,                // Line number.
        [PI_CURRENT_LINE_FILL] = 0x40000000,          // Current line fill.
        [PI_CURRENT_LINE_FILL_INACTIVE] = 0x40808080, // Current line fill (inactive).
        [PI_CURRENT_LINE_EDGE] = 0x40000000,          // Current line edge.
        [PI_LINE_EDITED] = 0xff84f2ef,                // Line edited.
        [PI_LINE_EDITED_SAVED] = 0xff307457,          // Line edited saved.
        [PI_LINE_EDITED_REVERTED] = 0xfffa955f,       // Line edited reverted.
    },
};

uint32_t ansi_palette[ANSI_MAX] = {
#undef ANSICOLOR
#define ANSICOLOR(C,RGB) [ANSI_##C] = RGB,
    ANSICOLORS(ANSICOLOR)
#undef ANSICOLOR
};
