/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "sv.h"
#include "widget.h"
#include <ctype.h>
#include <math.h>

#include <allocate.h>

#include <app/buffer.h>
#include <app/c.h>
#include <app/eddy.h>
#include <app/editor.h>
#include <app/listbox.h>
#include <app/minibuffer.h>
#include <app/palette.h>

DECLARE_SHARED_ALLOCATOR(eddy);

DA_IMPL(BufferView);
WIDGET_CLASS_DEF(Gutter, gutter);
WIDGET_CLASS_DEF(Editor, editor);
SIMPLE_WIDGET_CLASS_DEF(BufferView, view);

// -- Gutter -----------------------------------------------------------------

void gutter_init(Gutter *gutter)
{
    gutter->policy = SP_CHARACTERS;
    gutter->policy_size = 4;
    gutter->padding = DEFAULT_PADDING;
    gutter->background = palettes[PALETTE_DARK][PI_BACKGROUND];
}

void gutter_resize(Gutter *)
{
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
            palettes[PALETTE_DARK][PI_LINE_NUMBER]);
    }
}

void gutter_process_input(Gutter *)
{
}

// -- BufferView -------------------------------------------------------------

void view_init(BufferView *view)
{
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
    assert(view);
    editor->current_buffer = view_ix;
    view->cursor_flash = app->time;
    app->focus = (Widget *) editor;
    if (view->mode) {
        app->focus = view->mode;
    }
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
    view->cursor_pos.y = iclamp(view->cursor_pos.y + count, 0, buffer->lines.size - 1);
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

bool editor_character(Editor *editor, int ch)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    int         at = view->new_cursor;
    if (view->selection != -1) {
        at = view->new_cursor = editor_delete_selection(editor);
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

void editor_cmd_up(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    editor_manage_selection(editor, view, ctx->trigger.modifier & KMOD_SHIFT);
    editor_lines_up(editor, 1);
}

void editor_cmd_select_word(CommandContext *ctx)
{
    editor_select_word((Editor *) ctx->target);
}

void editor_cmd_down(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    editor_manage_selection(editor, view, ctx->trigger.modifier & KMOD_SHIFT);
    editor_lines_down(editor, 1);
}

void editor_cmd_left(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    editor_manage_selection(editor, view, ctx->trigger.modifier & KMOD_SHIFT);
    if (view->new_cursor > 0) {
        --view->new_cursor;
    }
    view->cursor_col = -1;
}

void editor_cmd_word_left(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    editor_manage_selection(editor, view, ctx->trigger.modifier & KMOD_SHIFT);
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

void editor_cmd_right(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    editor_manage_selection(editor, view, ctx->trigger.modifier & KMOD_SHIFT);
    if (view->new_cursor < buffer->text.view.length - 1) {
        ++view->new_cursor;
    }
    view->cursor_col = -1;
}

void editor_cmd_word_right(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    editor_manage_selection(editor, view, ctx->trigger.modifier & KMOD_SHIFT);
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

void editor_cmd_begin_of_line(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    assert(view->cursor_pos.y < buffer->lines.size);
    Index *line = buffer->lines.elements + view->cursor_pos.y;
    view->new_cursor = line->index_of;
    view->cursor_col = -1;
}

void editor_cmd_top_of_buffer(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    view->new_cursor = 0;
    view->cursor_col = -1;
    view->top_line = 0;
    view->left_column = 0;
}

void editor_cmd_end_of_line(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    assert(view->cursor_pos.y < buffer->lines.size);
    Index *line = buffer->lines.elements + view->cursor_pos.y;
    view->new_cursor = line->index_of + line->line.length;
    view->cursor_col = -1;
}

void editor_cmd_page_up(CommandContext *ctx)
{
    Editor *editor = (Editor *) ctx->target;
    editor_lines_up(editor, editor->lines);
}

void editor_cmd_page_down(CommandContext *ctx)
{
    Editor *editor = (Editor *) ctx->target;
    editor_lines_down(editor, editor->lines);
}

void editor_cmd_top(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    view->new_cursor = 0;
    view->cursor_col = -1;
}

void editor_cmd_bottom(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    view->new_cursor = buffer->text.view.length;
    view->cursor_col = -1;
}

void editor_cmd_split_line(CommandContext *ctx)
{
    Editor *editor = (Editor *) ctx->target;
    editor_character(editor, '\n');
}

void editor_cmd_merge_lines(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    Index      *line = buffer->lines.elements + view->cursor_pos.y;
    view->new_cursor = line->index_of + line->line.length;
    buffer_merge_lines(buffer, view->cursor_pos.y);
    view->cursor_col = -1;
}

#define OPEN_BRACES "({["
#define CLOSE_BRACES ")}]"
#define BRACES OPEN_BRACES CLOSE_BRACES

void _find_closing_brace(Editor *editor, size_t index, bool selection)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    int         brace = buffer->text.view.ptr[index];
    int         matching = 0;
    for (size_t ix = 0; ix < 3; ++ix) {
        if (OPEN_BRACES[ix] == brace) {
            matching = CLOSE_BRACES[ix];
            break;
        }
    }
    assert(matching);
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

void editor_cmd_matching_brace(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    bool        selection = ctx->trigger.modifier & KMOD_SHIFT;
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

void editor_cmd_backspace(CommandContext *ctx)
{
    Editor *editor = (Editor *) ctx->target;
    editor_backspace(editor);
}

void editor_cmd_delete_current_char(CommandContext *ctx)
{
    Editor *editor = (Editor *) ctx->target;
    editor_delete_current_char(editor);
}

void editor_cmd_clear_selection(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    view->selection = -1;
}

void editor_cmd_copy(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    if (view->selection == -1) {
        editor_select_line(editor);
    }
    editor_selection_to_clipboard(editor);
}

void editor_cmd_cut(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    if (view->selection == -1) {
        editor_select_line(editor);
    }
    editor_selection_to_clipboard(editor);
    view->new_cursor = editor_delete_selection(editor);
    view->cursor_col = -1;
    view->selection = -1;
}

void editor_cmd_paste(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
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

void editor_cmd_save_as(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    InputBox   *filename_box = inputbox_create(SV("New file name", 13), save_as_submit);
    filename_box->memo = editor;
    if (sv_not_empty(buffer->name)) {
        sb_append_sv(&filename_box->text, buffer->name);
    }
    inputbox_show(filename_box);
}

void editor_cmd_save(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    if (sv_empty(buffer->name)) {
        editor_cmd_save_as(ctx);
        return;
    }
    buffer_save(buffer);
    eddy_set_message(&eddy, "Buffer saved");
}

void editor_cmd_undo(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    buffer_undo(buffer);
}

void editor_cmd_redo(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    buffer_redo(buffer);
}

void switch_buffer_submit(ListBox *listbox, ListBoxEntry selection)
{
    size_t buffer_ix = (size_t) selection.payload;
    editor_select_buffer(eddy.editor, buffer_ix);
}

void editor_cmd_switch_buffer(CommandContext *ctx)
{
    Editor  *editor = (Editor *) ctx->target;
    ListBox *listbox = widget_new(ListBox);
    listbox->submit = (ListBoxSubmit) switch_buffer_submit;
    listbox->prompt = sv_from("Select buffer");
    for (size_t ix = 0; ix < eddy.buffers.size; ++ix) {
        Buffer    *buffer = eddy.buffers.elements + ix;
        StringView text;
        if (buffer->saved_version < buffer->version) {
            text = sv_printf("%.*s *", SV_ARG(buffer->name));
        } else {
            text = buffer->name;
        }
        da_append_ListBoxEntry(&listbox->entries, (ListBoxEntry) { text, (void *) ix });
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

void editor_cmd_close_buffer(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
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

void editor_cmd_close_view(CommandContext *ctx)
{
    Editor *editor = (Editor *) ctx->target;
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

void editor_cmd_find(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    sv_free(view->find_text);
    view->find_text = sv_null();
    sv_free(view->replacement);
    view->replacement = sv_null();
    minibuffer_query(ctx->target, SV("Find", 4), (MiniBufferQueryFunction) do_find);
}

void editor_cmd_find_next(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
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

void editor_cmd_find_replace(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    sv_free(view->find_text);
    view->find_text = sv_null();
    sv_free(view->replacement);
    view->replacement = sv_null();
    minibuffer_query(ctx->target, SV("Find", 4), (MiniBufferQueryFunction) do_find_query);
}

/*
 * ---------------------------------------------------------------------------
 * Life cycle
 * ---------------------------------------------------------------------------
 */

void editor_init(Editor *editor)
{
    editor->policy = SP_STRETCH;
    editor->background = palettes[PALETTE_DARK][PI_BACKGROUND];
    editor->padding = DEFAULT_PADDING;
    editor->num_clicks = 0;
    editor->handlers.character = (WidgetHandleCharacter) editor_character;
    widget_add_command(editor, sv_from("cursor-up"), editor_cmd_up,
        (KeyCombo) { KEY_UP, KMOD_NONE }, (KeyCombo) { KEY_UP, KMOD_SHIFT });
    widget_add_command(editor, sv_from("select-word"), editor_cmd_select_word,
        (KeyCombo) { KEY_UP, KMOD_ALT });
    widget_add_command(editor, sv_from("cursor-down"), editor_cmd_down,
        (KeyCombo) { KEY_DOWN, KMOD_NONE }, (KeyCombo) { KEY_DOWN, KMOD_SHIFT });
    widget_add_command(editor, sv_from("cursor-left"), editor_cmd_left,
        (KeyCombo) { KEY_LEFT, KMOD_NONE }, (KeyCombo) { KEY_LEFT, KMOD_SHIFT });
    widget_add_command(editor, sv_from("cursor-word-left"), editor_cmd_word_left,
        (KeyCombo) { KEY_LEFT, KMOD_ALT }, (KeyCombo) { KEY_LEFT, KMOD_ALT | KMOD_SHIFT });
    widget_add_command(editor, sv_from("cursor-right"), editor_cmd_right,
        (KeyCombo) { KEY_RIGHT, KMOD_NONE }, (KeyCombo) { KEY_RIGHT, KMOD_SHIFT });
    widget_add_command(editor, sv_from("cursor-word-right"), editor_cmd_word_right,
        (KeyCombo) { KEY_RIGHT, KMOD_ALT }, (KeyCombo) { KEY_LEFT, KMOD_ALT | KMOD_SHIFT });
    widget_add_command(editor, sv_from("cursor-page-up"), editor_cmd_page_up,
        (KeyCombo) { KEY_PAGE_UP, KMOD_NONE }, (KeyCombo) { KEY_PAGE_UP, KMOD_SHIFT },
        (KeyCombo) { KEY_UP, KMOD_SUPER }, (KeyCombo) { KEY_UP, KMOD_SUPER | KMOD_SHIFT });
    widget_add_command(editor, sv_from("cursor-page-down"), editor_cmd_page_down,
        (KeyCombo) { KEY_PAGE_DOWN, KMOD_NONE }, (KeyCombo) { KEY_PAGE_DOWN, KMOD_SHIFT },
        (KeyCombo) { KEY_DOWN, KMOD_SUPER }, (KeyCombo) { KEY_DOWN, KMOD_SUPER | KMOD_SHIFT });
    widget_add_command(editor, sv_from("cursor-home"), editor_cmd_begin_of_line,
        (KeyCombo) { KEY_HOME, KMOD_NONE }, (KeyCombo) { KEY_HOME, KMOD_SHIFT },
        (KeyCombo) { KEY_LEFT, KMOD_SUPER }, (KeyCombo) { KEY_LEFT, KMOD_SUPER | KMOD_SHIFT });
    widget_add_command(editor, sv_from("cursor-top"), editor_cmd_top_of_buffer,
        (KeyCombo) { KEY_HOME, KMOD_CONTROL }, (KeyCombo) { KEY_HOME, KMOD_SUPER | KMOD_SHIFT });
    widget_add_command(editor, sv_from("cursor-end"), editor_cmd_end_of_line,
        (KeyCombo) { KEY_END, KMOD_NONE }, (KeyCombo) { KEY_END, KMOD_SHIFT },
        (KeyCombo) { KEY_RIGHT, KMOD_SUPER }, (KeyCombo) { KEY_RIGHT, KMOD_SUPER | KMOD_SHIFT });
    widget_add_command(editor, sv_from("cursor-top"), editor_cmd_top,
        (KeyCombo) { KEY_HOME, KMOD_CONTROL }, (KeyCombo) { KEY_HOME, KMOD_CONTROL | KMOD_SHIFT });
    widget_add_command(editor, sv_from("cursor-bottom"), editor_cmd_bottom,
        (KeyCombo) { KEY_END, KMOD_CONTROL }, (KeyCombo) { KEY_END, KMOD_CONTROL | KMOD_SHIFT });
    widget_add_command(editor, sv_from("split-line"), editor_cmd_split_line,
        (KeyCombo) { KEY_ENTER, KMOD_NONE }, (KeyCombo) { KEY_KP_ENTER, KMOD_NONE });
    widget_add_command(editor, sv_from("merge-lines"), editor_cmd_merge_lines,
        (KeyCombo) { KEY_J, KMOD_SHIFT | KMOD_CONTROL });
    widget_add_command(editor, sv_from("matching-brace"), editor_cmd_matching_brace,
        (KeyCombo) { KEY_M, KMOD_CONTROL }, (KeyCombo) { KEY_M, KMOD_CONTROL | KMOD_SHIFT });
    widget_add_command(editor, sv_from("backspace"), editor_cmd_backspace,
        (KeyCombo) { KEY_BACKSPACE, KMOD_NONE });
    widget_add_command(editor, sv_from("delete-current-char"), editor_cmd_delete_current_char,
        (KeyCombo) { KEY_DELETE, KMOD_NONE });
    widget_add_command(editor, sv_from("clear-selection"), editor_cmd_clear_selection,
        (KeyCombo) { KEY_ESCAPE, KMOD_NONE });
    widget_add_command(editor, sv_from("copy-selection"), editor_cmd_copy,
        (KeyCombo) { KEY_C, KMOD_CONTROL });
    widget_add_command(editor, sv_from("cut-selection"), editor_cmd_cut,
        (KeyCombo) { KEY_X, KMOD_CONTROL });
    widget_add_command(editor, sv_from("paste-from-clipboard"), editor_cmd_paste,
        (KeyCombo) { KEY_V, KMOD_CONTROL });
    widget_add_command(editor, sv_from("editor-find"), editor_cmd_find,
        (KeyCombo) { KEY_F, KMOD_SUPER });
    widget_add_command(editor, sv_from("editor-find-next"), editor_cmd_find_next,
        (KeyCombo) { KEY_G, KMOD_SUPER });
    widget_add_command(editor, sv_from("editor-find-replace"), editor_cmd_find_replace,
        (KeyCombo) { KEY_R, KMOD_SUPER });
    widget_add_command(editor, sv_from("editor-save"), editor_cmd_save,
        (KeyCombo) { KEY_S, KMOD_CONTROL });
    widget_add_command(editor, sv_from("editor-save-as"), editor_cmd_save_as,
        (KeyCombo) { KEY_S, KMOD_CONTROL | KMOD_ALT });
    widget_add_command(editor, sv_from("editor-undo"), editor_cmd_undo,
        (KeyCombo) { KEY_Z, KMOD_CONTROL });
    widget_add_command(editor, sv_from("editor-redo"), editor_cmd_redo,
        (KeyCombo) { KEY_Z, KMOD_CONTROL | KMOD_SHIFT });
    widget_add_command(editor, sv_from("editor-switch-buffer"), editor_cmd_switch_buffer,
        (KeyCombo) { KEY_B, KMOD_SUPER });
    widget_add_command(editor, sv_from("editor-close-buffer"), editor_cmd_close_buffer,
        (KeyCombo) { KEY_W, KMOD_CONTROL });
    widget_add_command(editor, sv_from("editor-close-view"), editor_cmd_close_view,
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
    widget_draw_rectangle(editor, 0, 0, 0, 0, palettes[PALETTE_DARK][PI_BACKGROUND]);

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
                    palettes[PALETTE_DARK][PI_SELECTION]);
            }
        }
        if (line.num_tokens == 0) {
            if (frame == 0) {
                trace(CAT_EDIT, "%5d:%5zu:[          ]", row, lineno);
            }
            continue;
        }
        if (frame == 0) {
            trace_nonl(CAT_EDIT, "%5d:%5zu:[%4zu..%4zu]", row, lineno, line.first_token, line.first_token + line.num_tokens - 1);
        }
        for (size_t ix = line.first_token; ix < line.first_token + line.num_tokens; ++ix) {
            DisplayToken *token = buffer->tokens.elements + ix;
            int           start_col = (int) token->index - (int) line.index_of;
            if (start_col + (int) token->length <= (int) view->left_column) {
                continue;
            }
            if (start_col >= view->left_column + editor->columns) {
                break;
            }
            start_col = iclamp(start_col, view->left_column, start_col);
            int        length = iclamp((int) token->length, 0, editor->columns - start_col);
            StringView text = (StringView) { line.line.ptr + start_col, length };
            if (frame == 0) {
                trace_nonl(CAT_EDIT, "[%zu %.*s]", ix, SV_ARG(text));
            }
            widget_render_text(editor, eddy.cell.x * start_col, eddy.cell.y * row,
                text, eddy.font, palettes[PALETTE_DARK][token->color]);
        }
        if (frame == 0) {
            trace_nl(CAT_EDIT);
        }
    }

    if (view->mode != NULL && view->mode->handlers.draw != NULL) {
        view->mode->handlers.draw(view->mode);
    }

    double time = app->time - view->cursor_flash;
    if (time - floor(time) < 0.5) {
        int x = view->cursor_pos.x - view->left_column;
        int y = view->cursor_pos.y - view->top_line;
        widget_draw_rectangle(editor, x * eddy.cell.x, y * eddy.cell.y, 2, eddy.cell.y + 5, palettes[PALETTE_DARK][PI_CURSOR]);
    }
    DrawLine(editor->viewport.x + 80 * eddy.cell.x, editor->viewport.y,
        editor->viewport.x + 80 * eddy.cell.x, editor->viewport.y + editor->viewport.height,
        RAYWHITE);
    DrawLine(editor->viewport.x + 120 * eddy.cell.x, editor->viewport.y,
        editor->viewport.x + 120 * eddy.cell.x, editor->viewport.y + editor->viewport.height,
        RAYWHITE);
    ++frame;
}

void editor_process_input(Editor *editor)
{
    assert(editor->num_clicks >= 0 && editor->num_clicks < 3);
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && widget_contains(editor, GetMousePosition())) {
        BufferView *view = editor->buffers.elements + editor->current_buffer;
        Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
        int         line = imin((GetMouseY() - editor->viewport.y) / eddy.cell.y + view->top_line,
                    buffer->lines.size - 1);
        int         col = imin((GetMouseX() - editor->viewport.x) / eddy.cell.x + view->left_column,
                    buffer->lines.elements[line].line.length);
        view->new_cursor = buffer->lines.elements[line].index_of + col;
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
            size_t lineno = buffer_line_for_index(buffer, view->new_cursor);
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
