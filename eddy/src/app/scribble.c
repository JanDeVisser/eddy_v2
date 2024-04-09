/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <app/buffer.h>
#include <app/eddy.h>
#include <app/editor.h>
#include <app/scribble.h>
#include <app/widget.h>

// -- Scribble Mode ---------------------------------------------------------

MODE_CLASS_DEF(ScribbleMode, scribble_mode);

void scribble_mode_on_draw(ScribbleMode *mode)
{
}

void scribble_mode_buffer_event_listener(Buffer *buffer, BufferEvent event)
{
    switch (event.type) {
    case ETInsert:
    case ETDelete:
    case ETReplace:
    case ETIndexed:
    case ETSave:
    case ETClose:
    default:
        break;
    }
}

static int indent_for_line(Buffer *buffer, size_t lineno)
{
    while (lineno > 0) {
        --lineno;
        Index *l = buffer->lines.elements + lineno;
        int    non_space;
        int    indent = 0;
        for (non_space = 0; non_space < l->line.length && isspace(l->line.ptr[non_space]); ++non_space) {
            switch (l->line.ptr[non_space]) {
            case '\t':
                indent = ((indent / 4) + 1) * 4;
                break;
            default:
                ++indent;
            }
        }
        if (non_space < l->line.length) {
            int last_non_space;
            for (last_non_space = l->line.length; last_non_space >= 0 && isspace(l->line.ptr[last_non_space]); --last_non_space)
                ;
            if (l->line.ptr[last_non_space] == '{') {
                indent += 4;
            }
            return indent;
        }
        lineno--;
    }
    return 0;
}

void scribble_mode_cmd_split_line(ScribbleMode *mode, JSONValue unused)
{
    Editor     *editor = (Editor *) mode->parent->parent;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    size_t      lineno = buffer_line_for_index(buffer, view->new_cursor);
    int         indent_this_line = indent_for_line(buffer, lineno);
    int         indent_new_line = indent_this_line;
    Index      *l = buffer->lines.elements + lineno;
    size_t      index_of_next_line = l->index_of + l->line.length + 1;
    int         first_non_space, last_non_space;

    // | | | | |x| |=| |1|0|;| | |\n|
    // |}| | |\n|
    //  0 1 2 3 4 5 6 7 8 9 0 1 2 3
    //                          ^
    for (first_non_space = (int) l->index_of; buffer->text.ptr[first_non_space] != 0 && first_non_space < index_of_next_line && isspace(buffer->text.ptr[first_non_space]); ++first_non_space)
        ;
    for (last_non_space = (int) view->new_cursor - 1; last_non_space >= l->index_of && isspace(buffer->text.ptr[last_non_space]); --last_non_space)
        ;
    size_t text_length = 0;
    if (first_non_space >= last_non_space) {
        // Line is all whitespace. Reindent:
        if (l->line.length > 0) {
            buffer_delete(buffer, l->index_of, view->new_cursor - l->index_of);
        }
        if (indent_this_line > 0) {
            StringView s = sv_printf("%*s", indent_this_line, "");
            buffer_insert(buffer, s, (int) l->index_of);
            sv_free(s);
        }
    } else {
        text_length = last_non_space - first_non_space + 1;
        bool   last_is_close_curly = buffer->text.ptr[last_non_space] == '}';
        bool   last_is_open_curly = buffer->text.ptr[last_non_space] == '{';
        // Remove trailing whitespace:
        if (view->new_cursor > last_non_space) {
            // Actually strip the trailing whitespace:
            buffer_delete(buffer, last_non_space + 1, view->new_cursor - last_non_space - 1);
        }

        // Reindent:
        if (first_non_space > l->index_of) {
            buffer_delete(buffer, l->index_of, first_non_space - l->index_of);
        }
        if (last_is_close_curly) {
            indent_this_line -= 4;
            indent_new_line = indent_this_line;
        }
        if (indent_this_line > 0) {
            StringView s = sv_printf("%*s", indent_this_line, "");
            buffer_insert(buffer, s, l->index_of);
            sv_free(s);
        }
        if (last_is_open_curly) {
            indent_new_line += 4;
        }
    }

    StringView s = sv_printf("\n%*s", indent_new_line, "");
    buffer_insert(buffer, s, l->index_of + indent_this_line + text_length);
    sv_free(s);
    view->new_cursor = l->index_of + indent_this_line + text_length + 1 + indent_new_line;
    view->cursor_col = -1;
}

void scribble_mode_cmd_indent(ScribbleMode *mode, JSONValue unused)
{
    Editor     *editor = (Editor *) mode->parent->parent;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    size_t      lineno = buffer_line_for_index(buffer, view->new_cursor);
    int         indent_this_line = indent_for_line(buffer, lineno);
    Index      *l = buffer->lines.elements + lineno;
    int         first_non_space, last_non_space;

    // | | | | |x| |=| |1|0|;| | |\n|
    // |}| | |\n|
    //  0 1 2 3 4 5 6 7 8 9 0 1 2 3
    //                          ^
    for (first_non_space = l->index_of; buffer->text.ptr[first_non_space] != 0 && isspace(buffer->text.ptr[first_non_space]); ++first_non_space)
        ;
    for (last_non_space = view->new_cursor - 1; last_non_space >= l->index_of && isspace(buffer->text.ptr[last_non_space]); --last_non_space)
        ;
    size_t text_length = 0;
    if (first_non_space >= last_non_space) {
        // Line is all whitespace. Reindent:
        if (l->line.length > 0) {
            buffer_delete(buffer, l->index_of, view->new_cursor - l->index_of);
        }
        if (indent_this_line > 0) {
            StringView s = sv_printf("%*s", indent_this_line, "");
            buffer_insert(buffer, s, (int) l->index_of);
            sv_free(s);
        }
    } else {
        text_length = last_non_space - first_non_space + 1;
        bool last_is_close_curly = buffer->text.ptr[last_non_space] == '}';

        // Remove trailing whitespace:
        if (view->new_cursor > last_non_space) {
            // Actually strip the trailing whitespace:
            buffer_delete(buffer, last_non_space + 1, view->new_cursor - last_non_space - 1);
        }

        // Reindent:
        if (first_non_space > l->index_of) {
            buffer_delete(buffer, l->index_of, first_non_space - l->index_of);
        }
        if (last_is_close_curly) {
            indent_this_line -= 4;
        }
        if (indent_this_line > 0) {
            StringView s = sv_printf("%*s", indent_this_line, "");
            buffer_insert(buffer, s, l->index_of);
            sv_free(s);
        }
    }
    view->new_cursor = l->index_of + indent_this_line + text_length;
    view->cursor_col = -1;
}

void scribble_mode_cmd_unindent(ScribbleMode *mode, JSONValue unused)
{
    // Not sure how this is supposed to work
}

bool scribble_mode_character(ScribbleMode *mode, int ch)
{
    return false;
}

void scribble_mode_init(ScribbleMode *mode)
{
    widget_add_command(mode, "scribble-split-line", (WidgetCommandHandler) scribble_mode_cmd_split_line,
        (KeyCombo) { KEY_ENTER, KMOD_NONE }, (KeyCombo) { KEY_KP_ENTER, KMOD_NONE });
    widget_add_command(mode, "scribble-unindent", (WidgetCommandHandler) scribble_mode_cmd_unindent,
        (KeyCombo) { KEY_TAB, KMOD_SHIFT });
    // mode->handlers.on_draw = (WidgetOnDraw) scribble_mode_on_draw;
    BufferView *view = (BufferView *) mode->parent;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    buffer_add_listener(buffer, scribble_mode_buffer_event_listener);
}

typedef enum {
    ScribbleDirectiveStateInit = 0,
    ScribbleDirectiveStateIncludeQuote,
    ScribbleDirectiveStateMacroName,
} ScribbleDirectiveState;

static int handle_include_directive(Lexer *lexer)
{
    char const *buffer = lexer_source(lexer).ptr;
    size_t      ix = 0;
    size_t      state = (size_t) lexer->language_data;
    switch (state) {
    case ScribbleDirectiveStateInit: {
        while (buffer[ix] == ' ' || buffer[ix] == '\t') {
            ++ix;
        }
        lexer->language_data = (void *) ScribbleDirectiveStateIncludeQuote;
        if (ix > 0) {
            lexer_set_current(lexer, (Token) { .kind = TK_WHITESPACE, .text = { buffer, ix } });
            return ScribbleDirectiveInclude;
        }
        lexer->language_data = (void *) ScribbleDirectiveStateIncludeQuote;
    } // Fall through
    case ScribbleDirectiveStateIncludeQuote: {
        if (buffer[0] != '<' && buffer[0] != '"') {
            lexer->language_data = NULL;
            return NO_DIRECTIVE;
        }
        ++ix;
        while (buffer[ix] && buffer[ix] != '"' && buffer[ix] != '\n') {
            ++ix;
        }
        lexer_set_current(lexer, (Token) { .kind = TK_DIRECTIVE_ARG, .text = { buffer, ix + 1 } });
    } break;
    default:
        UNREACHABLE();
    }
    lexer->language_data = NULL;
    return NO_DIRECTIVE;
}

int handle_scribble_directive(Lexer *lexer, int directive)
{
    switch (directive) {
    case ScribbleDirectiveInclude:
        return handle_include_directive(lexer);
    default:
        lexer->language_data = (void *) NULL;
        return NO_DIRECTIVE;
    }
}
