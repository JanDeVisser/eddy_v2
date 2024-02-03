/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <math.h>

#include <allocate.h>

#include <buffer.h>
#include <eddy.h>
#include <editor.h>
#include <palette.h>

DECLARE_SHARED_ALLOCATOR(eddy);

DA_IMPL(BufferView);
WIDGET_CLASS_DEF(Gutter, gutter);
WIDGET_CLASS_DEF(Editor, editor);

// -- Gutter -----------------------------------------------------------------

void gutter_init(Gutter *gutter)
{
    gutter->policy = SP_CHARACTERS;
    gutter->policy_size = 4;
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

// -- Editor -----------------------------------------------------------------

/*
 * ---------------------------------------------------------------------------
 * Buffer Management
 * ---------------------------------------------------------------------------
 */

void editor_new(Editor *editor)
{
    Buffer        buffer = { 0 };
    StringBuilder name = { 0 };
    for (size_t num = 1; true; ++num) {
        sb_append_cstr(&name, TextFormat("untitled-%d", num));
        bool found = false;
        for (size_t ix = 0; ix < eddy.buffers.size; ++ix) {
            if (sv_eq(name.view, eddy.buffers.elements[ix].name)) {
                found = true;
                break;
            }
        }
        if (!found) {
            buffer.name = name.view;
            break;
        }
    }
    buffer.text = sb_create();
    buffer_build_indices(&buffer);
    da_append_Buffer(&eddy.buffers, buffer);
    editor_select_buffer(editor, eddy.buffers.size - 1);
}

void editor_select_buffer(Editor *editor, int buffer_num)
{
    for (size_t ix = 0; ix < editor->buffers.size; ++ix) {
        if (editor->buffers.elements[ix].buffer_num == buffer_num) {
            editor->current_buffer = ix;
            editor->buffers.elements[editor->current_buffer].cursor_flash = app->time;
            return;
        }
    }
    da_append_BufferView(&editor->buffers, (BufferView) { .buffer_num = buffer_num, .selection = -1 });
    editor->current_buffer = editor->buffers.size - 1;
    editor->buffers.elements[editor->current_buffer].cursor_flash = app->time;
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
        assert(view->new_cursor == -1);
        assert(view->cursor_col >= 0);
        if ((int) current_line->line.length < view->cursor_col) {
            view->cursor_pos.column = current_line->line.length - 1;
        } else {
            view->cursor_pos.column = view->cursor_col;
        }
        view->new_cursor = current_line->index_of + view->cursor_pos.column;
    } else {
        assert(view->new_cursor != -1);
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
        selection_start = imin(view->selection, view->cursor);
        int selection_end = imax(view->selection, view->cursor);
        editor_delete(editor, selection_start, selection_end - selection_start);
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

void editor_character(Editor *editor, int ch)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    int         at = view->cursor;
    if (view->selection != -1) {
        at = view->new_cursor = editor_delete_selection(editor);
    }
    editor_insert(editor, (StringView) { (char const *) &ch, 1 }, at);
    view->new_cursor = at + 1;
    view->cursor_col = -1;
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

void editor_cmd_end_of_line(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    assert(view->cursor_pos.y < buffer->lines.size);
    Index *line = buffer->lines.elements + view->cursor_pos.y;
    view->new_cursor = line->index_of + line->line.length - 1;
    view->cursor_col = -1;
    view->cursor_col = -1;
}

void editor_cmd_page_up(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
    editor_lines_up(editor, editor->lines);
}

void editor_cmd_page_down(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
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
    Editor     *editor = (Editor *) ctx->target;
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

void editor_cmd_backspace(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
    editor_backspace(editor);
}

void editor_cmd_delete_current_char(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
    editor_delete_current_char(editor);
}

void editor_cmd_clear_selection(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    view->selection = -1;
}

/*
 * ---------------------------------------------------------------------------
 * Life cycle
 * ---------------------------------------------------------------------------
 */

void editor_init(Editor *editor)
{
    editor->policy = SP_STRETCH;
    widget_add_command(editor, sv_from("cursor-up"), editor_cmd_up,
        (KeyCombo) { KEY_UP, KMOD_NONE }, (KeyCombo) { KEY_UP, KMOD_SHIFT });
    widget_add_command(editor, sv_from("cursor-down"), editor_cmd_down,
        (KeyCombo) { KEY_DOWN, KMOD_NONE }, (KeyCombo) { KEY_DOWN, KMOD_SHIFT });
    widget_add_command(editor, sv_from("cursor-left"), editor_cmd_left,
        (KeyCombo) { KEY_LEFT, KMOD_NONE }, (KeyCombo) { KEY_LEFT, KMOD_SHIFT });
    widget_add_command(editor, sv_from("cursor-right"), editor_cmd_right,
        (KeyCombo) { KEY_RIGHT, KMOD_NONE }, (KeyCombo) { KEY_RIGHT, KMOD_SHIFT });
    widget_add_command(editor, sv_from("cursor-page-up"), editor_cmd_page_up,
        (KeyCombo) { KEY_PAGE_UP, KMOD_NONE }, (KeyCombo) { KEY_PAGE_UP, KMOD_SHIFT });
    widget_add_command(editor, sv_from("cursor-page-down"), editor_cmd_page_down,
        (KeyCombo) { KEY_PAGE_DOWN, KMOD_NONE }, (KeyCombo) { KEY_PAGE_DOWN, KMOD_SHIFT });
    widget_add_command(editor, sv_from("cursor-home"), editor_cmd_begin_of_line,
        (KeyCombo) { KEY_HOME, KMOD_NONE }, (KeyCombo) { KEY_HOME, KMOD_SHIFT });
    widget_add_command(editor, sv_from("cursor-end"), editor_cmd_end_of_line,
        (KeyCombo) { KEY_END, KMOD_NONE }, (KeyCombo) { KEY_END, KMOD_SHIFT });
    widget_add_command(editor, sv_from("cursor-top"), editor_cmd_top,
        (KeyCombo) { KEY_HOME, KMOD_CONTROL }, (KeyCombo) { KEY_HOME, KMOD_CONTROL | KMOD_SHIFT });
    widget_add_command(editor, sv_from("cursor-bottom"), editor_cmd_bottom,
        (KeyCombo) { KEY_END, KMOD_CONTROL }, (KeyCombo) { KEY_END, KMOD_CONTROL | KMOD_SHIFT });
    widget_add_command(editor, sv_from("split-line"), editor_cmd_split_line,
        (KeyCombo) { KEY_ENTER, KMOD_NONE }, (KeyCombo) { KEY_KP_ENTER, KMOD_NONE });
    widget_add_command(editor, sv_from("merge-lines"), editor_cmd_merge_lines,
        (KeyCombo) { KEY_J, KMOD_SHIFT | KMOD_CONTROL });
    widget_add_command(editor, sv_from("backspace"), editor_cmd_backspace,
        (KeyCombo) { KEY_BACKSPACE, KMOD_NONE });
    widget_add_command(editor, sv_from("delete-current-char"), editor_cmd_delete_current_char,
        (KeyCombo) { KEY_DELETE, KMOD_NONE });
    widget_add_command(editor, sv_from("clear-selection"), editor_cmd_clear_selection,
        (KeyCombo) { KEY_ESCAPE, KMOD_NONE });
}

void editor_resize(Editor *editor)
{
    editor->columns = (int) ((editor->viewport.width - 2 * PADDING) / eddy.cell.x);
    editor->lines = (int) ((editor->viewport.height - 2 * PADDING) / eddy.cell.y);
}

void editor_draw(Editor *editor)
{
    editor_update_cursor(editor);
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    widget_draw_rectangle(editor, 0, 0, editor->viewport.width, editor->viewport.height,
        palettes[PALETTE_DARK][PI_BACKGROUND]);

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
            int    line_start = line.index_of + view->left_column;
            int    line_end = imin(line.index_of + line.line.length - 1, line_start + editor->columns);
            int    selection_offset = iclamp(selection_start - line_start, 0, line_end);

            if (selection_start < line_end && selection_end > line.index_of) {
                int width = selection_end - imax(selection_start, line_start);
                if (width > line_len - selection_offset) {
                    width = editor->columns - selection_offset;
                }
                widget_draw_rectangle(editor,
                    PADDING + eddy.cell.x * selection_offset, eddy.cell.y * row,
                    width * eddy.cell.x, eddy.cell.y + 5,
                    palettes[PALETTE_DARK][PI_SELECTION]);
            }
        }
        widget_render_text(editor, 0, eddy.cell.y * row,
            (StringView) { line.line.ptr + view->left_column, line_len },
            eddy.font, palettes[PALETTE_DARK][PI_DEFAULT]);
    }
    double time = app->time - view->cursor_flash;
    if (time - floor(time) < 0.5) {
        int x = view->cursor_pos.x - view->left_column;
        int y = view->cursor_pos.y - view->top_line;
        widget_draw_rectangle(editor, 5.0f + x * eddy.cell.x, 5.0f + y * eddy.cell.y, 2, eddy.cell.y, palettes[PALETTE_DARK][PI_CURSOR]);
    }
}

void editor_process_input(Editor *editor)
{
    for (int ch = GetCharPressed(); ch != 0; ch = GetCharPressed()) {
        editor_character(editor, ch);
    }
}
