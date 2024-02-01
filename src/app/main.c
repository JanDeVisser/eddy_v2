/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <math.h>
#include <raylib.h>

#include <allocate.h>
#include <io.h>

#include <buffer.h>
#include <widget.h>

#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 768

typedef struct {
    _W;
} Gutter;

WIDGET_CLASS(Gutter, gutter);

typedef struct {
    int        buffer_num;
    size_t     cursor;
    IntVector2 cursor_pos;
    int        cursor_col;
    size_t     new_cursor;
    int        top_line;
    int        left_column;
    size_t     selection;
    int        cursor_flash;
} BufferView;

DA_WITH_NAME(BufferView, BufferViews);

typedef struct {
    _W;
    BufferViews buffers;
    int         current_buffer;
    int         columns;
    int         lines;
} Editor;

WIDGET_CLASS(Editor, editor);
extern void editor_new(Editor *editor);
extern void editor_select_buffer(Editor *editor, int buffer_num);
extern void editor_update_cursor(Editor *editor);

typedef struct {
    _L;
} StatusBar;

LAYOUT_CLASS(StatusBar, sb);

typedef struct {
    _W;
    StringView message;
} MessageLine;

WIDGET_CLASS(MessageLine, message_line);

typedef struct {
    _A;
    Buffers buffers;
    Editor *editor;
} Eddy;

APP_CLASS(Eddy, eddy);

extern void eddy_on_draw(Eddy *eddy);
extern void eddy_open_buffer(Eddy *eddy, StringView file);
extern void eddy_set_message(Eddy *eddy, StringView message);
extern void eddy_clear_message(Eddy *eddy);

DECLARE_SHARED_ALLOCATOR(eddy);

Eddy eddy = { 0 };

SHARED_ALLOCATOR_IMPL(eddy);
DA_IMPL(BufferView);

WIDGET_CLASS_DEF(Gutter, gutter);

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
        widget_render_text(gutter, 0, eddy.cell.y * row, sv_from(TextFormat("%4d", lineno + 1)), BEIGE);
    }
}

void gutter_process_input(Gutter *)
{
}

WIDGET_CLASS_DEF(Editor, editor);

void editor_init(Editor *editor)
{
    editor->policy = SP_STRETCH;
}

void editor_resize(Editor *editor)
{
    editor->columns = (int) ((editor->viewport.width - 2 * PADDING) / eddy.cell.x);
    editor->lines = (int) ((editor->viewport.height - 2 * PADDING) / eddy.cell.y);
}

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
    view->cursor_flash = eddy.time;
}

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

bool _bv_in_selection(BufferView *view, Buffer *buffer, size_t point)
{
    if (view->selection == -1) {
        return false;
    }
    int selection_start = imin(view->selection, view->cursor);
    int selection_end = imax(view->selection, view->cursor);
    if (point < selection_start) {
        return false;
    }
    return point <= selection_end;
}

void editor_draw(Editor *editor)
{
    editor_update_cursor(editor);
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    widget_draw_rectangle(editor, 0, 0, editor->viewport.width, editor->viewport.height, BLACK);

    int selection_start = -1, selection_end = -1;
    if (view->selection != -1) {
        selection_start = imin(view->selection, view->cursor);
        selection_end = imax(view->selection, view->cursor);
    }

    for (int row = 0; row < editor->lines && view->top_line + row < buffer->lines.size; ++row) {
        size_t lineno = view->top_line + row;
        Index  line = buffer->lines.elements[lineno];
        int    line_len = imin(line.line.length - 1, view->left_column + editor->columns);
        int    line_start = line.index_of + view->left_column;
        int    line_end = imin(line.index_of + line.line.length, line_start + editor->columns);
        int    selection_offset = iclamp(selection_start - line_start, 0, line_end);
        int    end_selection_offset = iclamp(selection_end - line_start, 0, line_end);

        if (selection_offset < line_end && end_selection_offset > 0) {
            int width = iclamp(selection_end - selection_start, 0, line_len) * eddy.cell.x;
            if (width == line_len) {
                width = editor->columns * eddy.cell.x;
            }
            widget_draw_rectangle(editor,
                PADDING + eddy.cell.x * selection_offset, eddy.cell.y * row,
                width, eddy.cell.y + 1, (Color) { 0x1A, 0x30, 0x70, 0xFF });
        }
        widget_render_text(editor, 0, eddy.cell.y * row, (StringView) { line.line.ptr, line_len }, RAYWHITE);
    }
    double time = app->time - view->cursor_flash;
    if (time - floor(time) < 0.5) {
        int x = view->cursor_pos.x - view->left_column;
        int y = view->cursor_pos.y - view->top_line;
        widget_draw_rectangle(editor, 5.0f + x * eddy.cell.x, 5.0f + y * eddy.cell.y, 2, eddy.cell.y, RAYWHITE);
    }
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

void editor_up(Editor *editor)
{
    editor_lines_up(editor, 1);
}

void editor_down(Editor *editor)
{
    editor_lines_down(editor, 1);
}

void editor_left(Editor *editor)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    if (view->new_cursor > 0) {
        --view->new_cursor;
    }
    view->cursor_col = -1;
}

void editor_right(Editor *editor)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    if (view->new_cursor < buffer->text.view.length - 1) {
        ++view->new_cursor;
    }
    view->cursor_col = -1;
}

void editor_begin_of_line(Editor *editor)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    assert(view->cursor_pos.y < buffer->lines.size);
    Index *line = buffer->lines.elements + view->cursor_pos.y;
    view->new_cursor = line->index_of;
    view->cursor_col = -1;
}

void editor_end_of_line(Editor *editor)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    assert(view->cursor_pos.y < buffer->lines.size);
    Index *line = buffer->lines.elements + view->cursor_pos.y;
    view->new_cursor = line->index_of + line->line.length - 1;
    view->cursor_col = -1;
    view->cursor_col = -1;
}

void editor_page_up(Editor *editor)
{
    editor_lines_up(editor, editor->lines);
}

void editor_page_down(Editor *editor)
{
    editor_lines_down(editor, editor->lines);
}

void editor_top(Editor *editor)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    view->new_cursor = 0;
    view->cursor_col = -1;
}

void editor_bottom(Editor *editor)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    view->new_cursor = buffer->text.view.length;
    view->cursor_col = -1;
}

void editor_merge_lines(Editor *editor)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    Index      *line = buffer->lines.elements + view->cursor_pos.y;
    view->new_cursor = line->index_of + line->line.length;
    buffer_merge_lines(buffer, view->cursor_pos.y);
    view->cursor_col = -1;
}

int editor_clear_selection(Editor *editor)
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
        view->new_cursor = editor_clear_selection(editor);
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
        view->new_cursor = editor_clear_selection(editor);
        view->cursor_col = -1;
    }
}

void editor_character(Editor *editor, int ch)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    int         at = view->cursor;
    if (view->selection != -1) {
        at = view->new_cursor = editor_clear_selection(editor);
    }
    editor_insert(editor, (StringView) { (char const *) &ch, 1 }, at);
    view->new_cursor = at + 1;
    view->cursor_col = -1;
}

static void editor_start_selection(Editor *editor, BufferView *view)
{
    if (is_modifier_down(KMOD_SHIFT)) {
        if (view->selection == -1) {
            view->selection = view->cursor;
        }
    } else {
        view->selection = -1;
    }
}

void editor_process_input(Editor *editor)
{
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    view->new_cursor = view->cursor;

    if (is_key_pressed(KEY_UP, KMOD_NONE, KMOD_SHIFT)) {
        editor_start_selection(editor, view);
        editor_up(editor);
    }
    if (is_key_pressed(KEY_DOWN, KMOD_NONE, KMOD_SHIFT)) {
        editor_start_selection(editor, view);
        editor_down(editor);
    }
    if (is_key_pressed(KEY_LEFT, KMOD_NONE, KMOD_SHIFT)) {
        editor_start_selection(editor, view);
        editor_left(editor);
    }
    if (is_key_pressed(KEY_RIGHT, KMOD_NONE, KMOD_SHIFT)) {
        editor_start_selection(editor, view);
        editor_right(editor);
    }
    if (is_key_pressed(KEY_PAGE_UP, KMOD_NONE, KMOD_SHIFT)) {
        editor_start_selection(editor, view);
        editor_page_up(editor);
    }
    if (is_key_pressed(KEY_PAGE_DOWN, KMOD_NONE, KMOD_SHIFT)) {
        editor_start_selection(editor, view);
        editor_page_down(editor);
    }
    if (is_key_pressed(KEY_HOME, KMOD_NONE, KMOD_SHIFT)) {
        editor_start_selection(editor, view);
        editor_begin_of_line(editor);
    }
    if (is_key_pressed(KEY_HOME, KMOD_CONTROL, KMOD_CONTROL | KMOD_SHIFT)) {
        editor_start_selection(editor, view);
        editor_top(editor);
    }
    if (is_key_pressed(KEY_END, KMOD_NONE, KMOD_SHIFT)) {
        editor_start_selection(editor, view);
        editor_begin_of_line(editor);
    }
    if (is_key_pressed(KEY_END, KMOD_CONTROL, KMOD_CONTROL | KMOD_SHIFT)) {
        editor_start_selection(editor, view);
        editor_bottom(editor);
    }

    if (is_key_pressed(KEY_ENTER, KMOD_NONE) || is_key_pressed(KEY_KP_ENTER, KMOD_NONE)) {
        editor_character(editor, '\n');
    }
    if (is_key_pressed(KEY_J, KMOD_SHIFT | KMOD_CONTROL)) {
        editor_merge_lines(editor);
    }
    if (is_key_pressed(KEY_BACKSPACE, KMOD_NONE)) {
        editor_backspace(editor);
    }
    if (is_key_pressed(KEY_DELETE, KMOD_NONE)) {
        editor_delete_current_char(editor);
    }

    for (int ch = GetCharPressed(); ch != 0; ch = GetCharPressed()) {
        editor_character(editor, ch);
    }
}

LAYOUT_CLASS_DEF(StatusBar, sb);

void sb_file_name_draw(Label *label)
{
    if (!label->parent->memo) {
        label->parent->memo = layout_find_by_draw_function((Layout *) label->parent->parent, (WidgetDraw) editor_draw);
    }
    Editor *editor = (Editor *) label->parent->memo;
    assert(editor);
    BufferView *view = editor->buffers.elements + eddy.editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    label->text = buffer->name;
    label_draw(label);
}

void sb_cursor_draw(Label *label)
{
    if (!label->parent->memo) {
        label->parent->memo = layout_find_by_draw_function((Layout *) label->parent->parent, (WidgetDraw) editor_draw);
    }
    Editor *editor = (Editor *) label->parent->memo;
    assert(editor);
    BufferView *view = editor->buffers.elements + eddy.editor->current_buffer;
    if (view->selection != -1) {
        label->text = sv_from(TextFormat("%4d:%d %zu-%zu", view->cursor_pos.line + 1, view->cursor_pos.column + 1, view->selection, view->cursor));
    } else {
        label->text = sv_from(TextFormat("%4d:%d %zu", view->cursor_pos.line + 1, view->cursor_pos.column + 1, view->cursor));
    }
    label_draw(label);
}

void sb_last_key_draw(Label *label)
{
    label->text = sv_from(eddy.last_key);
    label_draw(label);
}

void sb_fps_draw(Label *label)
{
    int fps = GetFPS();
    label->text = sv_from(TextFormat("%d", fps));
    if (fps > 55) {
        label->color = GREEN;
    } else if (fps > 35) {
        label->color = ORANGE;
    } else {
        label->color = RED;
    }
    label_draw(label);
}

void sb_init(StatusBar *status_bar)
{
    status_bar->orientation = CO_HORIZONTAL;
    status_bar->policy = SP_CHARACTERS;
    status_bar->policy_size = 1.0f;
    layout_add_widget((Layout *) status_bar, widget_new_with_policy(Spacer, SP_CHARACTERS, 1));
    Label *file_name = (Label *) widget_new(Label);
    file_name->policy_size = 64;
    file_name->handlers.draw = (WidgetDraw) sb_file_name_draw;
    layout_add_widget((Layout *) status_bar, (Widget *) file_name);
    layout_add_widget((Layout *) status_bar, widget_new(Spacer));
    Label *cursor = (Label *) widget_new(Label);
    cursor->policy_size = 16;
    cursor->handlers.draw = (WidgetDraw) sb_cursor_draw;
    layout_add_widget((Layout *) status_bar, (Widget *) cursor);
    Label *last_key = (Label *) widget_new(Label);
    last_key->policy_size = 16;
    last_key->handlers.draw = (WidgetDraw) sb_last_key_draw;
    layout_add_widget((Layout *) status_bar, (Widget *) last_key);
    Label *fps = (Label *) widget_new(Label);
    fps->policy_size = 4;
    fps->handlers.draw = (WidgetDraw) sb_fps_draw;
    layout_add_widget((Layout *) status_bar, (Widget *) fps);
}

WIDGET_CLASS_DEF(MessageLine, message_line);

void message_line_init(MessageLine *message_line)
{
    message_line->policy = SP_CHARACTERS;
    message_line->policy_size = 1.0f;
    message_line->message = sv_null();
}

void message_line_resize(MessageLine *message_line)
{
}

void message_line_draw(MessageLine *message_line)
{
    widget_draw_rectangle(message_line, 0, 0, message_line->viewport.width, message_line->viewport.height, BLACK);
    if (!sv_empty(message_line->message)) {
        widget_render_text(message_line, 0, 0, message_line->message, RAYWHITE);
    }
}

void message_line_process_input(MessageLine *message_line)
{
}

APP_CLASS_DEF(Eddy, eddy);

void eddy_init(Eddy *eddy)
{
    Layout *editor_pane = layout_new(CO_HORIZONTAL);
    editor_pane->policy = SP_STRETCH;
    layout_add_widget(editor_pane, widget_new(Gutter));
    layout_add_widget(editor_pane, widget_new(Editor));

    Layout *main_area = layout_new(CO_VERTICAL);
    main_area->policy = SP_STRETCH;
    layout_add_widget(main_area, (Widget *) editor_pane);
    Widget *sb = widget_new(StatusBar);
    layout_add_widget(main_area, widget_new(StatusBar));
    layout_add_widget(main_area, widget_new(MessageLine));

    layout_add_widget((Layout *) eddy, (Widget *) main_area);
    eddy->editor = (Editor *) layout_find_by_draw_function((Layout *) eddy, (WidgetDraw) editor_draw);
    if (eddy->argc > 1) {
        for (int ix = 1; ix < eddy->argc; ++ix) {
            eddy_open_buffer(eddy, sv_from(app->argv[ix]));
        }
    } else {
        editor_new(eddy->editor);
    }
    editor_select_buffer(eddy->editor, 0);
    eddy->handlers.on_draw = (WidgetDraw) eddy_on_draw;
    app_init((App *) eddy);
}

void eddy_on_draw(Eddy *eddy)
{
    for (size_t ix = 0; ix < eddy->buffers.size; ++ix) {
        Buffer *buffer = eddy->buffers.elements + ix;
        if (buffer->rebuild_needed) {
            buffer_build_indices(buffer);
        }
    }
}

void eddy_open_buffer(Eddy *eddy, StringView file)
{
    Buffer buffer = { 0 };
    buffer.name = file;
    buffer.text.view = MUST(StringView, read_file_by_name(file));
    buffer.rebuild_needed = true;
    da_append_Buffer(&eddy->buffers, buffer);
}

void eddy_set_message(Eddy *eddy, StringView message)
{
    MessageLine *message_line = (MessageLine *) layout_find_by_draw_function((Layout *) eddy, (WidgetDraw) message_line_draw);
    assert(message_line);
    if (!sv_empty(message_line->message)) {
        sv_free(message_line->message);
    }
    message_line->message = message;
}

void eddy_clear_message(Eddy *eddy)
{
    MessageLine *message_line = (MessageLine *) layout_find_by_draw_function((Layout *) eddy, (WidgetDraw) message_line_draw);
    assert(message_line);
    if (!sv_empty(message_line->message)) {
        sv_free(message_line->message);
    }
}

int main(int argc, char **argv)
{
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "eddy");
    SetWindowMonitor(GetMonitorCount() - 1);
    SetWindowState(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_MAXIMIZED | FLAG_VSYNC_HINT);
    Image icon = LoadImage("eddy.png");
    SetWindowIcon(icon);
    SetExitKey(KEY_NULL);

    SetTargetFPS(60);
    MaximizeWindow();

    app_initialize((App *) &eddy, (WidgetInit) eddy_init, argc, argv);

    while (!WindowShouldClose()) {

        if (IsKeyPressed(KEY_Q) && (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))) {
            break;
        }

        layout_process_input((Layout *) &eddy);

        BeginDrawing();
        ClearBackground(DARKGRAY);
        layout_draw((Layout *) &eddy);
        EndDrawing();
    }
    UnloadFont(eddy.font);
    CloseWindow();

    return 0;
}
