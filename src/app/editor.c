/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <ctype.h>
#include <math.h>

#include <allocate.h>

#include <buffer.h>
#include <eddy.h>
#include <editor.h>
#include <json.h>
#include <lsp/lsp.h>
#include <palette.h>
#include <process.h>

DECLARE_SHARED_ALLOCATOR(eddy);

DA_IMPL(BufferView);
WIDGET_CLASS_DEF(Gutter, gutter);
WIDGET_CLASS_DEF(Editor, editor);
SIMPLE_WIDGET_CLASS_DEF(BufferView, view);

// -- C Mode ----------------------------------------------------------------

typedef struct {
    _W;
} CMode;

SIMPLE_WIDGET_CLASS(CMode, c_mode);
SIMPLE_WIDGET_CLASS_DEF(CMode, c_mode);

void c_mode_cmd_format_source(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target->parent;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    eddy_set_message(&eddy, ctx->called_as);

    if (sv_not_empty(buffer->name)) {
        StringView formatted = MUST(
           StringView,
           execute_pipe(
               buffer->text.view,
               sv_from("clang-format"),
               TextFormat("--assume-filename=%.*s", SV_ARG(buffer->name)),
               TextFormat("--cursor=%d", view->cursor)));
        StringView header = sv_chop_to_delim(&formatted, sv_from("\n"));
        if (sv_eq(formatted, buffer->text.view)) {
            eddy_set_message(&eddy, sv_from("Already properly formatted"));
            return;
        }
        JSONValue values = MUST(JSONValue, json_decode(header));
        assert(values.type == JSON_TYPE_OBJECT);
        sv_free(buffer->text.view);
        buffer->text.view = sv_copy(formatted);
        sv_free(formatted);
        buffer->rebuild_needed = true;
        view->new_cursor = json_get_int(&values, "Cursor", view->cursor);
        view->cursor_col = -1;
        view->selection = -1;
        eddy_set_message(&eddy, sv_from("Formatted"));
        return;
    }
    eddy_set_message(&eddy, sv_from("Cannot format untitled buffer"));
}

void c_mode_init(CMode *mode)
{
    widget_add_command(mode, sv_from("c-format-source"), c_mode_cmd_format_source,
        (KeyCombo) { KEY_L, KMOD_SHIFT | KMOD_CONTROL });
    BufferView *view = (BufferView*) mode->parent;
    Buffer *buffer = eddy.buffers.elements + view->buffer_num;
    lsp_on_open(buffer->name);
}

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
    buffer->text = sb_create();
    buffer->rebuild_needed = true;
    editor_select_buffer(editor, eddy.buffers.size - 1);
}

void editor_select_buffer(Editor *editor, int buffer_num)
{
    Buffer *buffer = eddy.buffers.elements + buffer_num;
    app->focus = (Widget *) editor;
    editor->current_buffer = -1;
    BufferView *view = NULL;
    for (size_t ix = 0; ix < editor->buffers.size; ++ix) {
        if (editor->buffers.elements[ix].buffer_num == buffer_num) {
            editor->current_buffer = ix;
            view = da_element_BufferView(&editor->buffers, ix);
            break;
        }
    }
    if (editor->current_buffer == -1) {
        view = da_append_BufferView(&editor->buffers, (BufferView) { .buffer_num = buffer_num, .selection = -1 });
        in_place_widget(BufferView, view, editor);
        editor->current_buffer = editor->buffers.size - 1;
        if (sv_endswith(buffer->name, sv_from(".c")) || sv_endswith(buffer->name, sv_from(".h"))) {
            view->mode = widget_new_with_parent(CMode, view);
        }
    }
    assert(view);
    view->cursor_flash = app->time;
    if (view->mode) {
        app->focus = view->mode;
    }
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

void editor_select_line(Editor *editor)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    size_t      lineno = buffer_line_for_index(buffer, view->cursor);
    Index      *line = buffer->lines.elements + lineno;
    view->selection = line->index_of;
    view->new_cursor = view->cursor = line->index_of + line->line.length;
}

void editor_word_left(Editor *editor)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    while (0 < view->cursor && !isalnum(buffer->text.view.ptr[view->cursor])) {
        ++view->cursor;
    }
    while (0 < view->cursor && isalnum(buffer->text.view.ptr[view->cursor])) {
        ++view->cursor;
    }
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

void editor_insert_string(Editor *editor, StringView sv)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    int         at = view->cursor;
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
        int     selection_start = imin(view->selection, view->cursor);
        int     selection_end = imax(view->selection, view->cursor);
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
        while (0 < view->new_cursor && !isalnum(buffer->text.view.ptr[view->new_cursor])) {
            --view->new_cursor;
        }
        while (0 < view->new_cursor && isalnum(buffer->text.view.ptr[view->new_cursor])) {
            --view->new_cursor;
        }
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

void editor_cmd_save(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    buffer_save(buffer);
    eddy_set_message(&eddy, sv_from("Buffer saved"));
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
    widget_add_command(editor, sv_from("editor-save"), editor_cmd_save,
        (KeyCombo) { KEY_S, KMOD_CONTROL });
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
            int line_start = line.index_of + view->left_column;
            int line_end = imin(line.index_of + line.line.length - 1, line_start + editor->columns);
            int selection_offset = iclamp(selection_start - line_start, 0, line_end);

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
    if ((IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) && widget_contains(editor, GetMousePosition())) {
        BufferView *view = editor->buffers.elements + editor->current_buffer;
        Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
        int         line = imin((GetMouseY() - editor->viewport.y) / eddy.cell.y + view->top_line,
                    buffer->lines.size - 1);
        int         col = imin((GetMouseX() - editor->viewport.x) / eddy.cell.x + view->left_column,
                    buffer->lines.elements[line].line.length - 1);
        view->new_cursor = view->cursor = buffer->lines.elements[line].index_of + col;
        view->cursor_col = -1;
        if (IsMouseButtonUp(MOUSE_BUTTON_LEFT)) {
            if (eddy.time - editor->clicks[editor->num_clicks] > 0.5) {
                editor->num_clicks = 0;
            }
            editor->clicks[editor->num_clicks] = eddy.time;
            ++editor->num_clicks;
            switch (++editor->num_clicks) {
            case 1:
                view->selection = view->cursor;
                break;
            case 2:
                editor_select_word(editor);
                break;
            case 3:
                editor_select_line(editor);
                // Fall through
            default:
                editor->num_clicks = 0;
            }
        }
    }
    for (int ch = GetCharPressed(); ch != 0; ch = GetCharPressed()) {
        editor_character(editor, ch);
    }
}
