/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <base/io.h>
#include <lang/scribble.h>

int scribble_handle_directive(Lexer *lexer, int directive)
{
    switch (directive) {
    case ScribbleDirectiveInclude: {
        StringView source = lexer_source(lexer);
        size_t     ix = 0;
        while (!isspace(source.ptr[ix]))
            ++ix;
        StringView file = { source.ptr, ix };
        StringView buffer = MUST(StringView, read_file_by_name(file));
        lexer_advance_source(lexer, ix);
        lexer_push_source(lexer, buffer, file);
        return NO_DIRECTIVE;
    default:
        UNREACHABLE();
    }
    }
}
