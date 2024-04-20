/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <ctype.h>
#include <math.h>

#include <app/buffer.h>
#include <app/c.h>
#include <app/eddy.h>
#include <app/editor.h>
#include <app/listbox.h>
#include <app/minibuffer.h>
#include <app/scribble.h>
#include <lsp/schema/SemanticTokens.h>

DA_IMPL(BufferView);
WIDGET_CLASS_DEF(Gutter, gutter);
WIDGET_CLASS_DEF(Editor, editor);
SIMPLE_WIDGET_CLASS_DEF(BufferView, view);

#define OPEN_BRACES "({["
#define CLOSE_BRACES ")}]"
#define BRACES OPEN_BRACES CLOSE_BRACES

int get_closing_brace_code(int brace);

// -- Gutter -----------------------------------------------------------------

void gutter_init(Gutter *gutter)
{
    gutter->policy = SP_CHARACTERS;
    gutter->policy_size = 5;
    gutter->padding = DEFAULT_PADDING;
    gutter->background = colour_to_color(eddy.theme.gutter.bg);
}

void gutter_resize(Gutter *gutter)
{
    gutter->background = colour_to_color(eddy.theme.gutter.bg);
}

void draw_diagnostic_float(Gutter *gutter)
{
    if (!gutter->memo) {
        gutter->memo = layout_find_by_draw_function((Layout *) gutter->parent, (WidgetDraw) editor_draw);
    }
    Editor     *editor = (Editor *) gutter->memo;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;

    StringList hover_text = { 0 };
    for (size_t ix = gutter->first_diagnostic_hover; ix < gutter->num_diagnostics_hover; ++ix) {
        sl_push(&hover_text, buffer->diagnostics.elements[gutter->first_diagnostic_hover + ix].message);
    }
    if (hover_text.size > 0) {
        widget_draw_hover_panel(gutter, gutter->viewport.width - 3, eddy.cell.y * gutter->row_diagnostic_hover + 6, hover_text,
            colour_to_color(eddy.theme.editor.bg), colour_to_color(eddy.theme.editor.fg));
    }
}

void gutter_draw(Gutter *gutter)
{
    if (!gutter->memo) {
        gutter->memo = layout_find_by_draw_function((Layout *) gutter->parent, (WidgetDraw) editor_draw);
    }
    Editor     *editor = (Editor *) gutter->memo;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    for (int row = 0; row < eddy.editor->lines && view->top_line + row < buffer->lines.size; ++row) {
        size_t lineno = view->top_line + row;
        widget_render_text(gutter, 0, eddy.cell.y * row,
            sv_from(TextFormat("%4d", lineno + 1)),
            eddy.font,
            colour_to_color(eddy.theme.gutter.fg));
        Index *line = buffer->lines.elements + lineno;
        if (line->num_diagnostics > 0) {
            widget_draw_rectangle(gutter, -6, eddy.cell.y * row, 6, eddy.cell.y, RED);
        }
    }
    if (gutter->num_diagnostics_hover > 0) {
        app_draw_floating(app, gutter, (WidgetDraw) draw_diagnostic_float);
    }
}

void gutter_process_input(Gutter *gutter)
{
    gutter->row_diagnostic_hover = 0;
    gutter->first_diagnostic_hover = 0;
    gutter->num_diagnostics_hover = 0;
    Vector2 mouse = GetMousePosition();
    if (widget_contains(gutter, mouse)) {
        IntVector2 gutter_coords = MUST_OPTIONAL(IntVector2, widget_coordinates(gutter, mouse));
        int        row = gutter_coords.y / eddy.cell.y;
        if (!gutter->memo) {
            gutter->memo = layout_find_by_draw_function((Layout *) gutter->parent, (WidgetDraw) editor_draw);
        }
        Editor     *editor = (Editor *) gutter->memo;
        BufferView *view = editor->buffers.elements + editor->current_buffer;
        Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
        size_t      lineno = view->top_line + row;
        Index      *line = buffer->lines.elements + lineno;
        if (line->num_diagnostics > 0) {
            gutter->row_diagnostic_hover = row;
            gutter->first_diagnostic_hover = line->first_diagnostic;
            gutter->num_diagnostics_hover = line->num_diagnostics;
        }
    }
}

// -- BufferView -------------------------------------------------------------

void view_init(BufferView *view)
{
    //
}

// -- Editor -----------------------------------------------------------------

/*
 * ---------------------------------------------------------------------------
 * Buffer Management
 * ---------------------------------------------------------------------------
 */

void editor_new(Editor *editor)
{
    Buffer *buffer = eddy_new_buffer(&eddy);
    editor_select_buffer(editor, buffer->buffer_ix);
}

ErrorOrInt editor_open(Editor *editor, StringView file)
{
    Buffer *buffer = TRY_TO(Buffer, Int, eddy_open_buffer(&eddy, file));
    editor_select_buffer(editor, buffer->buffer_ix);
    RETURN(Int, editor->current_buffer);
}

void editor_select_view(Editor *editor, int view_ix)
{
    assert(view_ix >= 0 && view_ix < editor->buffers.size);
    BufferView *view = editor->buffers.elements + view_ix;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    assert(view);
    editor->current_buffer = view_ix;
    view->cursor_flash = app->time;
    app->focus = (Widget *) editor;
    if (view->mode) {
        app->focus = view->mode;
    }
    SetWindowTitle(sv_cstr(buffer->name, NULL));
}

void editor_select_buffer(Editor *editor, int buffer_num)
{
    Buffer *buffer = eddy.buffers.elements + buffer_num;
    app->focus = (Widget *) editor;
    editor->current_buffer = -1;
    BufferView *view = NULL;
    for (size_t ix = 0; ix < editor->buffers.size; ++ix) {
        if (editor->buffers.elements[ix].buffer_num == buffer_num) {
            editor_select_view(editor, ix);
            return;
        }
    }
    int view_ix = 0;
    for (size_t ix = 0; ix < editor->buffers.size; ++ix) {
        if (editor->buffers.elements[ix].buffer_num == -1) {
            view = editor->buffers.elements + ix;
            view_ix = ix;
            break;
        }
    }
    if (!view) {
        view_ix = editor->buffers.size;
        view = da_append_BufferView(&editor->buffers, (BufferView) { .buffer_num = buffer_num, .selection = -1 });
    }
    in_place_widget(BufferView, view, editor);
    if (sv_endswith(buffer->name, sv_from(".c")) || sv_endswith(buffer->name, sv_from(".h"))) {
        view->mode = (Widget *) widget_new_with_parent(CMode, view);
        ++buffer->version;
    } else if (sv_endswith(buffer->name, sv_from(".scribble"))) {
        view->mode = (Widget *) widget_new_with_parent(ScribbleMode, view);
        ++buffer->version;
    }
    editor_select_view(editor, view_ix);
}

bool editor_has_prev(Editor *editor)
{
    int prev = editor->current_buffer - 1;
    while (prev >= 0 && editor->buffers.elements[prev].buffer_num == -1) {
        --prev;
    }
    return prev >= 0;
}

bool editor_has_next(Editor *editor)
{
    int next = editor->current_buffer + 1;
    while (next < editor->buffers.size && editor->buffers.elements[next].buffer_num == -1) {
        ++next;
    }
    return next < editor->buffers.size;
}

void editor_select_prev(Editor *editor)
{
    if (!editor_has_prev(editor)) {
        return;
    }
    int prev = editor->current_buffer - 1;
    while (prev >= 0 && editor->buffers.elements[prev].buffer_num == -1) {
        --prev;
    }
    if (prev >= 0) {
        editor_select_view(editor, prev);
    }
}

void editor_select_next(Editor *editor)
{
    int next = editor->current_buffer + 1;
    while (next < editor->buffers.size && editor->buffers.elements[next].buffer_num == -1) {
        ++next;
    }
    if (next < editor->buffers.size) {
        editor_select_view(editor, next);
    }
}

void editor_close_view(Editor *editor)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Widget     *mode = view->mode;
    if (mode && mode->handlers.on_terminate) {
        mode->handlers.on_terminate(mode);
    }
    memset(view, 0, sizeof(BufferView));
    if (editor->current_buffer == editor->buffers.size - 1) {
        --editor->buffers.size;
    } else {
        view->buffer_num = -1;
    }
    int current = editor->current_buffer;
    editor_select_prev(editor);
    if (editor->current_buffer == current) {
        editor_select_next(editor);
    }
    if (editor->current_buffer == current) {
        editor_new(editor);
    }
}

void editor_close_buffer(Editor *editor)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    int         buffer_num = view->buffer_num;
    editor_close_view(editor);
    eddy_close_buffer(&eddy, buffer_num);
}

/*
 * ---------------------------------------------------------------------------
 * Cursor manipulation
 * ---------------------------------------------------------------------------
 */

void editor_manage_selection(Editor *editor, BufferView *view, bool selection)
{
    if (selection) {
        if (view->selection == -1) {
            view->selection = view->cursor;
        }
    } else {
        view->selection = -1;
    }
}

void editor_update_cursor(Editor *editor)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    if (view->new_cursor == view->cursor) {
        return;
    }

    Buffer *buffer = eddy.buffers.elements + view->buffer_num;
    Index  *current_line = NULL;
    if (view->new_cursor != -1) {
        view->cursor_pos.line = buffer_line_for_index(buffer, view->new_cursor);
    }
    current_line = buffer->lines.elements + view->cursor_pos.line;
    if (view->new_cursor == -1) {
        assert(view->cursor_col >= 0);
        if (((int) current_line->line.length) <= (view->cursor_col - 1)) {
            view->cursor_pos.column = current_line->line.length;
        } else {
            view->cursor_pos.column = view->cursor_col;
        }
        view->new_cursor = current_line->index_of + view->cursor_pos.column;
    } else {
        view->cursor_pos.column = view->new_cursor - current_line->index_of;
    }
    view->cursor = view->new_cursor;

    if (view->cursor_pos.line < view->top_line) {
        view->top_line = view->cursor_pos.line;
    }
    if (view->cursor_pos.line >= view->top_line + editor->lines) {
        view->top_line = view->cursor_pos.line - editor->lines + 1;
    }
    if (view->cursor_pos.column < view->left_column) {
        view->left_column = view->cursor_pos.column;
    }
    if (view->cursor_pos.column >= view->left_column + editor->columns) {
        view->left_column = view->cursor_pos.column - editor->columns + 1;
    }
    view->cursor_flash = 0;
}

void editor_lines_up(Editor *editor, int count)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    if (view->cursor_pos.y == 0) {
        return;
    }
    view->new_cursor = -1;
    if (view->cursor_col < 0) {
        view->cursor_col = view->cursor_pos.x;
    }
    view->cursor_pos.y = iclamp(view->cursor_pos.y - count, 0, view->cursor_pos.y);
}

void editor_lines_down(Editor *editor, int count)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    if (view->cursor_pos.y >= buffer->lines.size - 1) {
        return;
    }
    view->new_cursor = -1;
    if (view->cursor_col < 0) {
        view->cursor_col = view->cursor_pos.x;
    }
    view->cursor_pos.y = iclamp(view->cursor_pos.y + count, 0, imax(0, buffer->lines.size - 1));
}

void editor_goto(Editor *editor, int line, int col)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    view->new_cursor = -1;
    view->cursor_pos.y = iclamp(line, 0, buffer->lines.size - 1);
    view->cursor_col = iclamp(col, 0, imax(0, buffer->lines.elements[view->cursor_pos.y].line.length - 1));
}

void editor_select_line(Editor *editor)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    size_t      lineno = buffer_line_for_index(buffer, view->cursor);
    Index      *line = buffer->lines.elements + lineno;
    view->selection = line->index_of;
    view->new_cursor = view->cursor = line->index_of + line->line.length + 1;
}

void editor_word_left(Editor *editor)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    while (0 < ((int) view->cursor) && !isalnum(buffer->text.view.ptr[view->cursor])) {
        ++view->cursor;
    }
    while (0 < ((int) view->cursor) && isalnum(buffer->text.view.ptr[view->cursor])) {
        ++view->cursor;
    }
    ++view->cursor;
}

void editor_word_right(Editor *editor)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    while (view->cursor < buffer->text.view.length - 1 && !isalnum(buffer->text.view.ptr[view->cursor])) {
        ++view->cursor;
    }
    while (view->cursor < buffer->text.view.length - 1 && isalnum(buffer->text.view.ptr[view->cursor])) {
        ++view->cursor;
    }
}

void editor_select_word(Editor *editor)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    view->selection = buffer_word_boundary_left(buffer, view->cursor);
    view->new_cursor = buffer_word_boundary_right(buffer, view->cursor);
}

/*
 * ---------------------------------------------------------------------------
 * Text manipulation
 * ---------------------------------------------------------------------------
 */

void editor_insert(Editor *editor, StringView text, size_t at)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    buffer_insert(buffer, text, at);
}

void editor_delete(Editor *editor, size_t at, size_t count)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    buffer_delete(buffer, at, count);
    view->new_cursor = at;
    view->cursor_col = -1;
    view->selection = -1;
}

int editor_delete_selection(Editor *editor)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    int         selection_start = -1;
    if (view->selection != -1) {
        selection_start = imin(view->selection, view->new_cursor);
        int selection_end = imax(view->selection, view->new_cursor);
        editor_delete(editor, selection_start, selection_end - selection_start);
        view->new_cursor = selection_start;
        view->selection = -1;
    }
    return selection_start;
}

void editor_backspace(Editor *editor)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    if (view->selection == -1) {
        if (view->cursor != 0) {
            editor_delete(editor, view->cursor - 1, 1);
            view->new_cursor = view->cursor - 1;
            view->cursor_col = -1;
        }
    } else {
        view->new_cursor = editor_delete_selection(editor);
        view->cursor_col = -1;
    }
}

void editor_delete_current_char(Editor *editor)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    if (view->selection == -1) {
        Buffer *buffer = eddy.buffers.elements + view->buffer_num;
        if (view->cursor < buffer->text.view.length) {
            editor_delete(editor, view->cursor, 1);
            view->new_cursor = view->cursor;
            view->cursor_col = -1;
        }
    } else {
        view->new_cursor = editor_delete_selection(editor);
        view->cursor_col = -1;
    }
}

int get_closing_brace_code(int brace)
{
    for (size_t ix = 0; ix < 3; ++ix) {
        if (OPEN_BRACES[ix] == brace) {
            return CLOSE_BRACES[ix];
        }
    }
    return -1;
}

bool editor_character(Editor *editor, int ch)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    size_t         at = view->new_cursor;
    if (view->selection != -1) {
        switch (ch) {
        case '(':
        case '[':
        case '{': {
            int close = get_closing_brace_code(ch);
            int selection_start = imin((int) view->selection, (int) view->new_cursor);
            int selection_end = imax((int) view->selection, (int) view->new_cursor);
            editor_insert(editor, (StringView) { (char const *) &ch, 1 }, selection_start);
            editor_insert(editor, (StringView) { (char const *) &close, 1 }, selection_end + 1);
            if (view->cursor == selection_end) {
                view->new_cursor = selection_end + 1;
                view->selection = selection_start + 1;
            } else {
                view->new_cursor = selection_start + 1;
                view->selection = selection_end + 1;
            }
            view->cursor_col = -1;
            return true;
        }
        default:
            at = view->new_cursor = (size_t) editor_delete_selection(editor);
        }
    }
    editor_insert(editor, (StringView) { (char const *) &ch, 1 }, at);
    view->new_cursor = at + 1;
    view->cursor_col = -1;
    return true;
}

void editor_insert_string(Editor *editor, StringView sv)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    int         at = view->new_cursor;
    if (view->selection != -1) {
        at = view->new_cursor = editor_delete_selection(editor);
    }
    editor_insert(editor, sv, at);
    view->new_cursor = at + sv.length;
    view->cursor_col = -1;
}

void editor_selection_to_clipboard(Editor *editor)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    if (view->selection != -1) {
        Buffer *buffer = eddy.buffers.elements + view->buffer_num;
        int     selection_start = imin(view->selection, view->new_cursor);
        int     selection_end = imax(view->selection, view->new_cursor);
        char    ch = buffer->text.view.ptr[selection_end];
        ((char *) buffer->text.view.ptr)[selection_end] = 0;
        SetClipboardText(buffer->text.view.ptr + selection_start);
        ((char *) buffer->text.view.ptr)[selection_end] = ch;
    }
}

/*
 * ---------------------------------------------------------------------------
 * Commands
 * ---------------------------------------------------------------------------
 */

bool do_select(JSONValue key_combo)
{
    if (key_combo.type == JSON_TYPE_BOOLEAN) {
        return key_combo.boolean;
    }
    assert(key_combo.type == JSON_TYPE_OBJECT);
    KeyboardModifier modifier = (KeyboardModifier) json_get_int(&key_combo, "modifier", KMOD_NONE);
    return (modifier & KMOD_SHIFT) != 0;
}

void editor_cmd_up(Editor *editor, JSONValue key_combo)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    editor_manage_selection(editor, view, do_select(key_combo));
    editor_lines_up(editor, 1);
}

void editor_cmd_select_word(Editor *editor, JSONValue unused)
{
    editor_select_word(editor);
}

void editor_cmd_down(Editor *editor, JSONValue key_combo)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    editor_manage_selection(editor, view, do_select(key_combo));
    editor_lines_down(editor, 1);
}

void editor_cmd_left(Editor *editor, JSONValue key_combo)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    editor_manage_selection(editor, view, do_select(key_combo));
    if (view->new_cursor > 0) {
        --view->new_cursor;
    }
    view->cursor_col = -1;
}

void editor_cmd_word_left(Editor *editor, JSONValue key_combo)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    editor_manage_selection(editor, view, do_select(key_combo));
    if (view->new_cursor > 0) {
        Buffer *buffer = eddy.buffers.elements + view->buffer_num;
        while (0 < ((int) view->new_cursor) && !isalnum(buffer->text.view.ptr[view->new_cursor])) {
            --view->new_cursor;
        }
        while (0 < ((int) view->new_cursor) && isalnum(buffer->text.view.ptr[view->new_cursor])) {
            --view->new_cursor;
        }
        view->new_cursor = ((int) view->new_cursor >= 0) ? view->new_cursor + 1 : 0;
    }
    view->cursor_col = -1;
}

void editor_cmd_right(Editor *editor, JSONValue key_combo)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    editor_manage_selection(editor, view, do_select(key_combo));
    if (view->new_cursor < buffer->text.view.length - 1) {
        ++view->new_cursor;
    }
    view->cursor_col = -1;
}

void editor_cmd_word_right(Editor *editor, JSONValue key_combo)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    editor_manage_selection(editor, view, do_select(key_combo));
    size_t len = buffer->text.view.length;
    if (view->new_cursor < len - 1) {
        while (view->new_cursor < len - 1 && !isalnum(buffer->text.view.ptr[view->new_cursor])) {
            ++view->new_cursor;
        }
        while (view->new_cursor < len - 1 && isalnum(buffer->text.view.ptr[view->new_cursor])) {
            ++view->new_cursor;
        }
    }
    view->cursor_col = -1;
}

void editor_cmd_begin_of_line(Editor *editor, JSONValue unused)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    assert(view->cursor_pos.y < buffer->lines.size);
    Index *line = buffer->lines.elements + view->cursor_pos.y;
    view->new_cursor = line->index_of;
    view->cursor_col = -1;
}

void editor_cmd_top_of_buffer(Editor *editor, JSONValue unused)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    view->new_cursor = 0;
    view->cursor_col = -1;
    view->top_line = 0;
    view->left_column = 0;
}

void editor_cmd_end_of_line(Editor *editor, JSONValue unused)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    assert(view->cursor_pos.y < buffer->lines.size);
    Index *line = buffer->lines.elements + view->cursor_pos.y;
    view->new_cursor = line->index_of + line->line.length;
    view->cursor_col = -1;
}

void editor_cmd_page_up(Editor *editor, JSONValue unused)
{
    editor_lines_up(editor, editor->lines);
}

void editor_cmd_page_down(Editor *editor, JSONValue unused)
{
    editor_lines_down(editor, editor->lines);
}

void editor_cmd_top(Editor *editor, JSONValue unused)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    view->new_cursor = 0;
    view->cursor_col = -1;
}

void editor_cmd_bottom(Editor *editor, JSONValue unused)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    view->new_cursor = buffer->text.view.length;
    view->cursor_col = -1;
}

void editor_cmd_split_line(Editor *editor, JSONValue unused)
{
    editor_character(editor, '\n');
}

void editor_cmd_merge_lines(Editor *editor, JSONValue unused)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    Index      *line = buffer->lines.elements + view->cursor_pos.y;
    view->new_cursor = line->index_of + line->line.length;
    buffer_merge_lines(buffer, view->cursor_pos.y);
    view->cursor_col = -1;
}

void _find_closing_brace(Editor *editor, size_t index, bool selection)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    int         brace = buffer->text.view.ptr[index];
    int         matching = get_closing_brace_code(brace);
    assert(matching > 0);
    if (selection) {
        view->selection = index;
    }
    int depth = 1;
    while (++index < buffer->text.view.length) {
        if (buffer->text.view.ptr[index] == matching) {
            --depth;
        }
        if (!depth) {
            view->new_cursor = ++index;
            view->cursor_col = -1;
            return;
        }
        if (buffer->text.view.ptr[index] == brace) {
            ++depth;
        }
    }
}

void _find_opening_brace(Editor *editor, size_t index, bool selection)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    int         brace = buffer->text.view.ptr[index];
    int         matching = 0;
    for (size_t ix = 0; ix < 3; ++ix) {
        if (CLOSE_BRACES[ix] == brace) {
            matching = OPEN_BRACES[ix];
            break;
        }
    }
    if (selection) {
        view->selection = index;
    }
    assert(matching);
    int depth = 1;
    while (--index != -1) {
        if (buffer->text.view.ptr[index] == matching) {
            --depth;
        }
        if (!depth) {
            view->new_cursor = index;
            view->cursor_col = -1;
            return;
        }
        if (buffer->text.view.ptr[index] == brace) {
            ++depth;
        }
    }
}

void editor_cmd_matching_brace(Editor *editor, JSONValue key_combo)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    bool        selection = do_select(key_combo);
    if (strchr(OPEN_BRACES, buffer->text.view.ptr[view->cursor])) {
        _find_closing_brace(editor, view->cursor, selection);
        return;
    }
    if (strchr(OPEN_BRACES, buffer->text.view.ptr[view->cursor - 1])) {
        _find_closing_brace(editor, view->cursor - 1, selection);
        return;
    }
    if (strchr(CLOSE_BRACES, buffer->text.view.ptr[view->cursor])) {
        _find_opening_brace(editor, view->cursor, selection);
        return;
    }
    if (strchr(CLOSE_BRACES, buffer->text.view.ptr[view->cursor - 1])) {
        _find_opening_brace(editor, view->cursor - 1, selection);
        return;
    }
}

void editor_cmd_backspace(Editor *editor, JSONValue unused)
{
    editor_backspace(editor);
}

void editor_cmd_delete_current_char(Editor *editor, JSONValue unused)
{
    editor_delete_current_char(editor);
}

void editor_cmd_clear_selection(Editor *editor, JSONValue unused)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    view->selection = -1;
}

void editor_cmd_copy(Editor *editor, JSONValue unused)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    if (view->selection == -1) {
        editor_select_line(editor);
    }
    editor_selection_to_clipboard(editor);
}

void editor_cmd_cut(Editor *editor, JSONValue unused)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    if (view->selection == -1) {
        editor_select_line(editor);
    }
    editor_selection_to_clipboard(editor);
    view->new_cursor = editor_delete_selection(editor);
    view->cursor_col = -1;
    view->selection = -1;
}

void editor_cmd_paste(Editor *editor, JSONValue unused)
{
    char const *text = GetClipboardText();
    editor_insert_string(editor, sv_from(text));
}

void save_as_submit(InputBox *filename_box, StringView filename)
{
    Editor     *editor = filename_box->memo;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    buffer_save_as(buffer, filename);
    eddy_set_message(&eddy, "Buffer saved");
}

void editor_cmd_save_as(Editor *editor, JSONValue unused)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    InputBox   *filename_box = inputbox_create(SV("New file name", 13), save_as_submit);
    filename_box->memo = editor;
    if (sv_not_empty(buffer->name)) {
        sb_append_sv(&filename_box->text, buffer->name);
    }
    inputbox_show(filename_box);
}

void editor_cmd_save(Editor *editor, JSONValue unused)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    if (sv_empty(buffer->name)) {
        editor_cmd_save_as(editor, unused);
        return;
    }
    buffer_save(buffer);
    eddy_set_message(&eddy, "Buffer saved");
}

void editor_cmd_undo(Editor *editor, JSONValue unused)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    buffer_undo(buffer);
}

void editor_cmd_redo(Editor *editor, JSONValue unused)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    buffer_redo(buffer);
}

void switch_buffer_submit(ListBox *listbox, ListBoxEntry selection)
{
    int buffer_ix = selection.int_value;
    editor_select_buffer(eddy.editor, buffer_ix);
}

void editor_cmd_switch_buffer(Editor *editor, JSONValue unused)
{
    ListBox *listbox = widget_new(ListBox);
    listbox->submit = (ListBoxSubmit) switch_buffer_submit;
    listbox->prompt = sv_from("Select buffer");
    for (int ix = 0; ix < eddy.buffers.size; ++ix) {
        Buffer    *buffer = eddy.buffers.elements + ix;
        StringView text;
        if (buffer->saved_version < buffer->version) {
            text = sv_printf("%.*s *", SV_ARG(buffer->name));
        } else {
            text = buffer->name;
        }
        da_append_ListBoxEntry(&listbox->entries, (ListBoxEntry) { .text = text, .int_value = ix });
    }
    listbox_show(listbox);
}

void editor_are_you_sure_handler(ListBox *are_you_sure, QueryOption selection)
{
    switch (selection) {
    case QueryOptionYes: {
        BufferView *view = eddy.editor->buffers.elements + eddy.editor->current_buffer;
        Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
        buffer_save(buffer);
    } // Fall through:
    case QueryOptionNo:
        editor_close_buffer(eddy.editor);
        break;
    default:
        // do nothing
        break;
    }
}

void editor_cmd_close_buffer(Editor *editor, JSONValue unused)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;

    StringView prompt = sv_null();
    if (sv_empty(buffer->name) && sv_not_empty(buffer->text.view)) {
        prompt = sv_printf("File is modified. Do you want to save it before closing?");
    }
    if (buffer->saved_version < buffer->version) {
        prompt = sv_printf("File '%.*s' is modified. Do you want to save it before closing?", SV_ARG(buffer->name));
    }
    if (sv_not_empty(prompt)) {
        ListBox *are_you_sure = listbox_create_query(prompt, editor_are_you_sure_handler, QueryOptionYesNoCancel);
        listbox_show(are_you_sure);
        return;
    }
    editor_close_buffer(editor);
}

void editor_cmd_close_view(Editor *editor, JSONValue unused)
{
    editor_close_view(editor);
}

bool find_next(BufferView *view)
{
    assert(sv_not_empty(view->find_text));
    Buffer *buffer = eddy.buffers.elements + view->buffer_num;
    int     pos = sv_find_from(buffer->text.view, view->find_text, view->new_cursor);
    if (pos < 0) {
        pos = sv_find(buffer->text.view, view->find_text);
    }
    if (pos >= 0) {
        view->selection = pos;
        view->new_cursor = pos + view->find_text.length;
        view->cursor_col = -1;
        return true;
    }
    return false;
}

MiniBufferChain do_find(Editor *editor, StringView query)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    sv_free(view->find_text);
    view->find_text = sv_copy(query);
    find_next(view);
    return (MiniBufferChain) { 0 };
}

void editor_cmd_find(Editor *editor, JSONValue unused)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    sv_free(view->find_text);
    view->find_text = sv_null();
    sv_free(view->replacement);
    view->replacement = sv_null();
    minibuffer_query(editor, SV("Find", 4), (MiniBufferQueryFunction) do_find);
}

void editor_cmd_find_next(Editor *editor, JSONValue unused)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    if (sv_empty(view->find_text)) {
        return;
    }
    find_next(view);
}

MiniBufferChain do_ask_replace(Editor *editor, StringView reply)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    char        cmd = 0;
    if (sv_not_empty(reply)) {
        cmd = toupper(reply.ptr[0]);
    }
    switch (cmd) {
    case 'Y': {
        editor_delete_selection(editor);
        editor_insert_string(editor, view->replacement);
    } break;
    case 'N':
        break;
    case 'A': {
        do {
            editor_delete_selection(editor);
            editor_insert_string(editor, view->replacement);
        } while (find_next(view));
        return (MiniBufferChain) { 0 };
    }
    case 'Q':
        return (MiniBufferChain) { 0 };
    default:
        return (MiniBufferChain) { .fnc = (MiniBufferQueryFunction) do_ask_replace, .prompt = SV("Replace ((Y)es/(N)o/(A)ll/(Q)uit)", 33) };
    }
    if (find_next(view)) {
        return (MiniBufferChain) { .fnc = (MiniBufferQueryFunction) do_ask_replace, .prompt = SV("Replace ((Y)es/(N)o/(A)ll/(Q)uit)", 33) };
    }
    eddy_set_message(&eddy, "Not found");
    return (MiniBufferChain) { 0 };
}

MiniBufferChain do_replacement_query(Editor *editor, StringView replacement)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    sv_free(view->replacement);
    view->replacement = sv_copy(replacement);
    if (find_next(view)) {
        return (MiniBufferChain) { .fnc = (MiniBufferQueryFunction) do_ask_replace, .prompt = SV("Replace ((Y)es/(N)o/(A)ll/(Q)uit)", 33) };
    }
    eddy_set_message(&eddy, "Not found");
    return (MiniBufferChain) { 0 };
}

MiniBufferChain do_find_query(Editor *editor, StringView query)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    sv_free(view->find_text);
    view->find_text = sv_copy(query);
    return (MiniBufferChain) { .fnc = (MiniBufferQueryFunction) do_replacement_query, .prompt = SV("Replace with", 12) };
}

void editor_cmd_find_replace(Editor *editor, JSONValue unused)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    sv_free(view->find_text);
    view->find_text = sv_null();
    sv_free(view->replacement);
    view->replacement = sv_null();
    minibuffer_query(editor, SV("Find", 4), (MiniBufferQueryFunction) do_find_query);
}

MiniBufferChain do_goto(Editor *editor, StringView query)
{
    StringList coords = sv_split(query, SV(":", 1));
    int        line = -1;
    int        col = -1;
    if (coords.size > 0) {
        IntegerParseResult line_maybe = sv_parse_u32(sv_strip(coords.strings[0]));
        if (line_maybe.success) {
            line = line_maybe.integer.i32;
            if (coords.size > 1) {
                IntegerParseResult col_maybe = sv_parse_u32(sv_strip(coords.strings[1]));
                if (line_maybe.success) {
                    col = (int) col_maybe.integer.u32;
                }
            }
        }
    }
    if (line >= 1) {
        editor_goto(editor, line - 1, (col >= 1) ? col - 1 : col);
    }
    return (MiniBufferChain) { 0 };
}

void editor_cmd_goto(Editor *editor, JSONValue unused)
{
    minibuffer_query(editor, SV("Move to", 7), (MiniBufferQueryFunction) do_goto);
}

/*
 * ---------------------------------------------------------------------------
 * Life cycle
 * ---------------------------------------------------------------------------
 */

void editor_init(Editor *editor)
{
    editor->policy = SP_STRETCH;
    editor->background = colour_to_color(eddy.theme.editor.bg);
    editor->padding = DEFAULT_PADDING;
    editor->num_clicks = 0;
    editor->handlers.character = (WidgetHandleCharacter) editor_character;
    widget_add_command(editor, "cursor-up", (WidgetCommandHandler) editor_cmd_up,
        (KeyCombo) { KEY_UP, KMOD_NONE }, (KeyCombo) { KEY_UP, KMOD_SHIFT });
    widget_add_command(editor, "select-word", (WidgetCommandHandler) editor_cmd_select_word,
        (KeyCombo) { KEY_UP, KMOD_ALT });
    widget_add_command(editor, "cursor-down", (WidgetCommandHandler) editor_cmd_down,
        (KeyCombo) { KEY_DOWN, KMOD_NONE }, (KeyCombo) { KEY_DOWN, KMOD_SHIFT });
    widget_add_command(editor, "cursor-left", (WidgetCommandHandler) editor_cmd_left,
        (KeyCombo) { KEY_LEFT, KMOD_NONE }, (KeyCombo) { KEY_LEFT, KMOD_SHIFT });
    widget_add_command(editor, "cursor-word-left", (WidgetCommandHandler) editor_cmd_word_left,
        (KeyCombo) { KEY_LEFT, KMOD_ALT }, (KeyCombo) { KEY_LEFT, KMOD_ALT | KMOD_SHIFT });
    widget_add_command(editor, "cursor-right", (WidgetCommandHandler) editor_cmd_right,
        (KeyCombo) { KEY_RIGHT, KMOD_NONE }, (KeyCombo) { KEY_RIGHT, KMOD_SHIFT });
    widget_add_command(editor, "cursor-word-right", (WidgetCommandHandler) editor_cmd_word_right,
        (KeyCombo) { KEY_RIGHT, KMOD_ALT }, (KeyCombo) { KEY_LEFT, KMOD_ALT | KMOD_SHIFT });
    widget_add_command(editor, "cursor-page-up", (WidgetCommandHandler) editor_cmd_page_up,
        (KeyCombo) { KEY_PAGE_UP, KMOD_NONE }, (KeyCombo) { KEY_PAGE_UP, KMOD_SHIFT },
        (KeyCombo) { KEY_UP, KMOD_SUPER }, (KeyCombo) { KEY_UP, KMOD_SUPER | KMOD_SHIFT });
    widget_add_command(editor, "cursor-page-down", (WidgetCommandHandler) editor_cmd_page_down,
        (KeyCombo) { KEY_PAGE_DOWN, KMOD_NONE }, (KeyCombo) { KEY_PAGE_DOWN, KMOD_SHIFT },
        (KeyCombo) { KEY_DOWN, KMOD_SUPER }, (KeyCombo) { KEY_DOWN, KMOD_SUPER | KMOD_SHIFT });
    widget_add_command(editor, "cursor-home", (WidgetCommandHandler) editor_cmd_begin_of_line,
        (KeyCombo) { KEY_HOME, KMOD_NONE }, (KeyCombo) { KEY_HOME, KMOD_SHIFT },
        (KeyCombo) { KEY_LEFT, KMOD_SUPER }, (KeyCombo) { KEY_LEFT, KMOD_SUPER | KMOD_SHIFT });
    widget_add_command(editor, "cursor-top", (WidgetCommandHandler) editor_cmd_top_of_buffer,
        (KeyCombo) { KEY_HOME, KMOD_CONTROL }, (KeyCombo) { KEY_HOME, KMOD_SUPER | KMOD_SHIFT });
    widget_add_command(editor, "cursor-end", (WidgetCommandHandler) editor_cmd_end_of_line,
        (KeyCombo) { KEY_END, KMOD_NONE }, (KeyCombo) { KEY_END, KMOD_SHIFT },
        (KeyCombo) { KEY_RIGHT, KMOD_SUPER }, (KeyCombo) { KEY_RIGHT, KMOD_SUPER | KMOD_SHIFT });
    widget_add_command(editor, "cursor-top", (WidgetCommandHandler) editor_cmd_top,
        (KeyCombo) { KEY_HOME, KMOD_CONTROL }, (KeyCombo) { KEY_HOME, KMOD_CONTROL | KMOD_SHIFT });
    widget_add_command(editor, "cursor-bottom", (WidgetCommandHandler) editor_cmd_bottom,
        (KeyCombo) { KEY_END, KMOD_CONTROL }, (KeyCombo) { KEY_END, KMOD_CONTROL | KMOD_SHIFT });
    widget_add_command(editor, "split-line", (WidgetCommandHandler) editor_cmd_split_line,
        (KeyCombo) { KEY_ENTER, KMOD_NONE }, (KeyCombo) { KEY_KP_ENTER, KMOD_NONE });
    widget_add_command(editor, "merge-lines", (WidgetCommandHandler) editor_cmd_merge_lines,
        (KeyCombo) { KEY_J, KMOD_SHIFT | KMOD_CONTROL });
    widget_add_command(editor, "matching-brace", (WidgetCommandHandler) editor_cmd_matching_brace,
        (KeyCombo) { KEY_M, KMOD_CONTROL }, (KeyCombo) { KEY_M, KMOD_CONTROL | KMOD_SHIFT });
    widget_add_command(editor, "backspace", (WidgetCommandHandler) editor_cmd_backspace,
        (KeyCombo) { KEY_BACKSPACE, KMOD_NONE });
    widget_add_command(editor, "delete-current-char", (WidgetCommandHandler) editor_cmd_delete_current_char,
        (KeyCombo) { KEY_DELETE, KMOD_NONE });
    widget_add_command(editor, "clear-selection", (WidgetCommandHandler) editor_cmd_clear_selection,
        (KeyCombo) { KEY_ESCAPE, KMOD_NONE });
    widget_add_command(editor, "copy-selection", (WidgetCommandHandler) editor_cmd_copy,
        (KeyCombo) { KEY_C, KMOD_SUPER });
    widget_add_command(editor, "cut-selection", (WidgetCommandHandler) editor_cmd_cut,
        (KeyCombo) { KEY_X, KMOD_SUPER });
    widget_add_command(editor, "paste-from-clipboard", (WidgetCommandHandler) editor_cmd_paste,
        (KeyCombo) { KEY_V, KMOD_SUPER });
    widget_add_command(editor, "editor-find", (WidgetCommandHandler) editor_cmd_find,
        (KeyCombo) { KEY_F, KMOD_SUPER });
    widget_add_command(editor, "editor-find-next", (WidgetCommandHandler) editor_cmd_find_next,
        (KeyCombo) { KEY_G, KMOD_SUPER });
    widget_add_command(editor, "editor-goto", (WidgetCommandHandler) editor_cmd_goto,
        (KeyCombo) { KEY_L, KMOD_SUPER });
    widget_add_command(editor, "editor-find-replace", (WidgetCommandHandler) editor_cmd_find_replace,
        (KeyCombo) { KEY_R, KMOD_SUPER });
    widget_add_command(editor, "editor-save", (WidgetCommandHandler) editor_cmd_save,
        (KeyCombo) { KEY_S, KMOD_CONTROL });
    widget_add_command(editor, "editor-save-as", (WidgetCommandHandler) editor_cmd_save_as,
        (KeyCombo) { KEY_S, KMOD_CONTROL | KMOD_ALT });
    widget_add_command(editor, "editor-undo", (WidgetCommandHandler) editor_cmd_undo,
        (KeyCombo) { KEY_Z, KMOD_SUPER });
    widget_add_command(editor, "editor-redo", (WidgetCommandHandler) editor_cmd_redo,
        (KeyCombo) { KEY_Z, KMOD_SUPER | KMOD_SHIFT });
    widget_add_command(editor, "editor-switch-buffer", (WidgetCommandHandler) editor_cmd_switch_buffer,
        (KeyCombo) { KEY_B, KMOD_SUPER });
    widget_add_command(editor, "editor-close-buffer", (WidgetCommandHandler) editor_cmd_close_buffer,
        (KeyCombo) { KEY_W, KMOD_CONTROL });
    widget_add_command(editor, "editor-close-view", (WidgetCommandHandler) editor_cmd_close_view,
        (KeyCombo) { KEY_W, KMOD_CONTROL | KMOD_SHIFT });
}

void editor_resize(Editor *editor)
{
    editor->columns = (int) ((editor->viewport.width - 2 * PADDING) / eddy.cell.x);
    editor->lines = (int) ((editor->viewport.height - 2 * PADDING) / eddy.cell.y);
}

void editor_draw(Editor *editor)
{
    static size_t frame = 1;
    editor_update_cursor(editor);
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    widget_draw_rectangle(editor, 0, 0, 0, 0, colour_to_color(eddy.theme.editor.bg));

    int selection_start = -1, selection_end = -1;
    if (view->selection != -1) {
        selection_start = imin(view->selection, view->cursor);
        selection_end = imax(view->selection, view->cursor);
    }

    for (int row = 0; row < editor->lines && view->top_line + row < buffer->lines.size; ++row) {
        size_t lineno = view->top_line + row;
        Index  line = buffer->lines.elements[lineno];
        int    line_len = imin(line.line.length - 1, view->left_column + editor->columns);
        if (view->selection != -1) {
            int line_start = line.index_of + view->left_column;
            int line_end = imin(line.index_of + line.line.length - 1, line_start + editor->columns);
            int selection_offset = iclamp(selection_start - line_start, 0, line_end);

            if (selection_start < line_end && selection_end > line.index_of) {
                int width = selection_end - imax(selection_start, line_start);
                if (width > line_len - selection_offset) {
                    width = editor->columns - selection_offset;
                }
                widget_draw_rectangle(editor,
                    eddy.cell.x * selection_offset, eddy.cell.y * row,
                    width * eddy.cell.x, eddy.cell.y + 5,
                    colour_to_color(eddy.theme.selection.bg));
            }
        }
        if (line.num_tokens == 0) {
            if (frame == 0) {
                trace(EDIT, "%5d:%5zu:[          ]", row, lineno);
            }
            continue;
        }
//        if (frame == 0) {
//            trace_nonl(EDIT, "%5d:%5zu:[%4zu..%4zu]", row, lineno, line.first_token, line.first_token + line.num_tokens - 1);
//        }
        for (size_t ix = line.first_token; ix < line.first_token + line.num_tokens; ++ix) {
            DisplayToken *token = buffer->tokens.elements + ix;
            int           start_col = (int) token->index - (int) line.index_of;

            // token ends before left edge
            if (start_col + (int) token->length <= (int) view->left_column) {
                continue;
            }

            // token starts after right edge. We're done here; go to next line
            if (start_col >= view->left_column + editor->columns) {
                break;
            }

            // Cut off at left edge
            if (start_col < view->left_column) {
                start_col = view->left_column;
            }

            // Length taking left edge into account
            int length = token->length - (start_col - ((int) token->index - (int) line.index_of));
            // If start + length overflows right edge, clip length:
            if (start_col + length > view->left_column + editor->columns) {
                length = view->left_column + editor->columns - start_col;
            }

            StringView text = (StringView) { line.line.ptr + start_col, length };
//            if (frame == 0) {
//                trace_nonl(EDIT, "[%zu %.*s]", ix, SV_ARG(text));
//            }
            widget_render_text(editor, eddy.cell.x * (start_col - view->left_column), eddy.cell.y * row,
                text, eddy.font, token->color);
        }
//        if (frame == 0) {
//            trace_nl(EDIT);
//        }
    }

    if (view->mode != NULL && view->mode->handlers.draw != NULL) {
        view->mode->handlers.draw(view->mode);
    }

    double time = app->time - view->cursor_flash;
    if (time - floor(time) < 0.5) {
        int x = view->cursor_pos.x - view->left_column;
        int y = view->cursor_pos.y - view->top_line;
        widget_draw_rectangle(editor, x * eddy.cell.x, y * eddy.cell.y, 2, eddy.cell.y + 1, colour_to_color(eddy.theme.editor.fg));
    }
    DrawLine(editor->viewport.x + 80 * eddy.cell.x, editor->viewport.y,
        editor->viewport.x + 80 * eddy.cell.x, editor->viewport.y + editor->viewport.height,
        colour_to_color(eddy.theme.editor.fg));
    DrawLine(editor->viewport.x + 120 * eddy.cell.x, editor->viewport.y,
        editor->viewport.x + 120 * eddy.cell.x, editor->viewport.y + editor->viewport.height,
        colour_to_color(eddy.theme.editor.fg));
    ++frame;
}

void editor_process_input(Editor *editor)
{
    assert(editor->num_clicks >= 0 && editor->num_clicks < 3);
    if (!widget_contains(editor, GetMousePosition())) {
        return;
    }
    float mouse_move = GetMouseWheelMove();
    if (mouse_move != 0.0) {
        if (IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER)) {
            eddy_inc_font_size(&eddy, (int) -mouse_move);
        } else {
            if (mouse_move < 0) {
                editor_lines_down(editor, (int) -mouse_move);
            }
            if (mouse_move > 0) {
                editor_lines_up(editor, (int) mouse_move);
            }
        }
        return;
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        BufferView *view = editor->buffers.elements + editor->current_buffer;
        Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
        int         lineno = imin((GetMouseY() - editor->viewport.y) / eddy.cell.y + view->top_line,
                    buffer->lines.size - 1);
        int         col = imin((GetMouseX() - editor->viewport.x) / eddy.cell.x + view->left_column,
                    buffer->lines.elements[lineno].line.length);
        view->new_cursor = buffer->lines.elements[lineno].index_of + col;
        view->cursor_col = -1;
        if (editor->num_clicks > 0 && (eddy.time - editor->clicks[editor->num_clicks - 1]) > 0.5) {
            editor->num_clicks = 0;
        }
        editor->clicks[editor->num_clicks] = eddy.time;
        ++editor->num_clicks;
        switch (editor->num_clicks) {
        case 1:
            view->selection = -1;
            break;
        case 2:
            view->selection = buffer_word_boundary_left(buffer, view->new_cursor);
            view->new_cursor = buffer_word_boundary_right(buffer, view->new_cursor);
            break;
        case 3: {
            lineno = buffer_line_for_index(buffer, view->new_cursor);
            Index *line = buffer->lines.elements + lineno;
            view->selection = line->index_of;
            view->new_cursor = line->index_of + line->line.length + 1;
        }
            // Fall through
        default:
            editor->num_clicks = 0;
        }
    }
    assert(editor->num_clicks >= 0 && editor->num_clicks < 3);
}
