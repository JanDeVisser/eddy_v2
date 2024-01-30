/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <math.h>
#include <raylib.h>

#include <allocate.h>
#include <io.h>

#include <widget.h>

#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 768

typedef struct {
    size_t     index_of;
    StringView line;
} Index;

DA_WITH_NAME(Index, Indices);
DA_IMPL(Index);

typedef struct {
    StringView    name;
    StringBuilder text;
    Indices       lines;
    size_t        cursor;
    IntVector2    cursor_pos;
    int           cursor_col;
    int           top_line;
    int           left_column;
} Buffer;

DA_WITH_NAME(Buffer, Buffers);
DA_IMPL(Buffer);

typedef struct {
    _W;
} Gutter;

WIDGET_CLASS(Gutter, gutter);

typedef struct {
    _W;
    Buffers    buffers;
    IntVector2 outline;
    int        current_buffer;
    double     cursor_flash;
    int        columns;
    int        lines;
} Editor;

WIDGET_CLASS(Editor, editor);
extern void editor_open_buffer(Editor *editor, StringView file);
extern void editor_new(Editor *editor);

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
    Editor *editor;
} Eddy;

APP_CLASS(Eddy, eddy);

DECLARE_SHARED_ALLOCATOR(eddy)
extern void eddy_set_message(Eddy *eddy, StringView message);
extern void eddy_clear_message(Eddy *eddy);

Eddy eddy = { 0 };

SHARED_ALLOCATOR_IMPL(eddy)

void buffer_build_indices(Buffer *buffer)
{
    buffer->lines.size = 0;
    if (buffer->text.view.length == 0) {
        return;
    }
    bool  new_line = false;
    Index index = { 0 };
    index.line = buffer->text.view;
    for (size_t ix = 0; ix < buffer->text.view.length; ++ix) {
        if (new_line) {
            index.line.length = ix - index.index_of;
            da_append_Index(&buffer->lines, index);
            index.index_of = ix;
            index.line.ptr = buffer->text.view.ptr + ix;
            index.line.length = buffer->text.view.length - ix;
        }
        new_line = buffer->text.view.ptr[ix] == '\n';
    }
    da_append_Index(&buffer->lines, index);
}

size_t buffer_line_for_index(Buffer *buffer, int index)
{
    assert(buffer != NULL);
    Indices *indices = &buffer->lines;
    if (indices->size == 0) {
        return 0;
    }
    size_t line_min = 0;
    size_t line_max = indices->size - 1;
    while (true) {
        size_t line = line_min + (line_max - line_min) / 2;
        if ((line < indices->size - 1 && indices->elements[line].index_of <= index && index < indices->elements[line + 1].index_of) || (line == indices->size - 1 && indices->elements[line].index_of <= index)) {
            return line;
        }
        if (indices->elements[line].index_of > index) {
            line_max = line;
        } else {
            line_min = line + 1;
        }
    }
}

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
    Buffer *buffer = eddy.editor->buffers.elements + eddy.editor->current_buffer;
    for (int row = 0; row < eddy.editor->lines && buffer->top_line + row < buffer->lines.size; ++row) {
        size_t lineno = buffer->top_line + row;
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
    for (int ix = 1; ix < eddy.argc; ++ix) {
        editor_open_buffer(editor, sv_from(app->argv[ix]));
    }
    if (editor->buffers.size == 0) {
        editor_new(editor);
    }
}

void editor_resize(Editor *editor)
{
    editor->cursor_flash = GetTime();
    editor->columns = (int) ((editor->viewport.width - 2 * PADDING) / eddy.cell.x);
    editor->lines = (int) ((editor->viewport.height - 2 * PADDING) / eddy.cell.y);
}

void editor_open_buffer(Editor *editor, StringView file)
{
    Buffer buffer = { 0 };
    buffer.name = file;
    buffer.text.view = MUST(StringView, read_file_by_name(file));
    buffer_build_indices(&buffer);
    da_append_Buffer(&editor->buffers, buffer);
    editor->current_buffer = editor->buffers.size - 1;
}

void editor_new(Editor *editor)
{
    Buffer        buffer = { 0 };
    StringBuilder name = { 0 };
    for (size_t num = 1; true; ++num) {
        sb_append_cstr(&name, TextFormat("untitled-%d", num));
        bool found = false;
        for (size_t ix = 0; ix < editor->buffers.size; ++ix) {
            if (sv_eq(name.view, editor->buffers.elements[ix].name)) {
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
    da_append_Buffer(&editor->buffers, buffer);
    editor->current_buffer = editor->buffers.size - 1;
}

void editor_draw(Editor *editor)
{
    Buffer *buffer = editor->buffers.elements + editor->current_buffer;
    widget_draw_rectangle(editor, 0, 0, editor->viewport.width, editor->viewport.height, BLACK);
    for (int row = 0; row < editor->lines && buffer->top_line + row < buffer->lines.size; ++row) {
        size_t lineno = buffer->top_line + row;
        Index  line = buffer->lines.elements[lineno];
        int    ix = imin(line.line.length - 1, buffer->left_column + editor->columns);
        widget_render_text(editor, 0, eddy.cell.y * row, (StringView) { line.line.ptr, ix }, RAYWHITE);
    }

    double time = app->time - editor->cursor_flash;
    if (time - floor(time) < 0.5) {
        int x = buffer->cursor_pos.x - buffer->left_column;
        int y = buffer->cursor_pos.y - buffer->top_line;
        widget_draw_rectangle(editor, 5.0f + x * eddy.cell.x, 5.0f + y * eddy.cell.y, 2, eddy.cell.y, RAYWHITE);
    }
}

void editor_process_input(Editor *editor)
{
    Buffer *buffer = editor->buffers.elements + editor->current_buffer;
    int     new_cursor = buffer->cursor;

    Index line = buffer->lines.elements[buffer->cursor_pos.y];
    if (IS_PRESSED(KEY_UP, KMOD_NONE)) {
        if (buffer->cursor_pos.y > 0) {
            new_cursor = -1;
            buffer->cursor_pos.y = buffer->cursor_pos.y - 1;
            if (buffer->cursor_col < 0) {
                buffer->cursor_col = buffer->cursor_pos.column;
            }
        }
    }

    if (IS_PRESSED(KEY_DOWN, KMOD_NONE)) {
        if (buffer->cursor_pos.y < buffer->lines.size - 1) {
            new_cursor = -1;
            buffer->cursor_pos.y = buffer->cursor_pos.y + 1;
            if (buffer->cursor_col < 0) {
                buffer->cursor_col = buffer->cursor_pos.column;
            }
        }
    }

    if (IS_PRESSED(KEY_LEFT, KMOD_NONE)) {
        if (new_cursor > 0) {
            --new_cursor;
            buffer->cursor_col = -1;
        }
    }

    if (IS_PRESSED(KEY_RIGHT, KMOD_NONE)) {
        if (new_cursor < buffer->text.view.length - 1) {
            ++new_cursor;
            buffer->cursor_col = -1;
        }
    }

    int rebuild_needed = 0;
    if (IS_PRESSED(KEY_ENTER, KMOD_NONE) || IS_PRESSED(KEY_KP_ENTER, KMOD_NONE)) {
        char c = '\n';
        sb_insert_chars(&buffer->text, &c, 1, buffer->cursor);
        ++new_cursor;
        buffer->cursor_pos.x = 0;
        ++buffer->cursor_pos.y;
        ++rebuild_needed;
    }

    if (IS_PRESSED(KEY_J, KMOD_SHIFT | KMOD_CONTROL)) {
        ((char *) buffer->text.view.ptr)[line.index_of + line.line.length - 1]  = ' ';
        new_cursor = line.index_of + line.line.length - 1;
        buffer->cursor_col = -1;
        ++rebuild_needed;
    }

    for (int ch = GetCharPressed(); ch != 0; ch = GetCharPressed()) {
        char c = (char) ch;
        sb_insert_chars(&buffer->text, &c, 1, buffer->cursor);
        ++new_cursor;
        buffer->cursor_col = -1;
        ++rebuild_needed;
    }

    if (rebuild_needed) {
        buffer_build_indices(buffer);
    }

    if (new_cursor != buffer->cursor) {
        Index *current_line = NULL;
        if (new_cursor != -1) {
            buffer->cursor_pos.line = buffer_line_for_index(buffer, new_cursor);
        }
        current_line = buffer->lines.elements + buffer->cursor_pos.line;
        if (new_cursor == -1) {
            assert(new_cursor == -1);
            assert(buffer->cursor_col >= 0);
            if ((int) current_line->line.length < buffer->cursor_col) {
                buffer->cursor_pos.column = current_line->line.length - 1;
            } else {
                buffer->cursor_pos.column = buffer->cursor_col;
            }
            new_cursor = current_line->index_of + buffer->cursor_pos.column;
        } else {
            assert(new_cursor != -1);
            buffer->cursor_pos.column = new_cursor - current_line->index_of;
        }
        buffer->cursor = new_cursor;

        if (buffer->cursor_pos.line < buffer->top_line) {
            buffer->top_line = buffer->cursor_pos.line;
        }
        if (buffer->cursor_pos.line >= buffer->top_line + editor->lines) {
            buffer->top_line = buffer->cursor_pos.line - editor->lines + 1;
        }
        if (buffer->cursor_pos.column < buffer->left_column) {
            buffer->left_column = buffer->cursor_pos.column;
        }
        if (buffer->cursor_pos.column >= buffer->left_column + editor->columns) {
            buffer->left_column = buffer->cursor_pos.column - editor->columns + 1;
        }
        editor->cursor_flash = eddy.time;
    }
}

LAYOUT_CLASS_DEF(StatusBar, sb);

void sb_file_name_draw(Label *label)
{
    Buffer *buffer = eddy.editor->buffers.elements + eddy.editor->current_buffer;
    label->text = buffer->name;
    label_draw(label);
}

void sb_cursor_draw(Label *label)
{
    Buffer *buffer = eddy.editor->buffers.elements + eddy.editor->current_buffer;
    label->text = sv_from(TextFormat("%4d:%d", buffer->cursor_pos.line + 1, buffer->cursor_pos.column + 1));
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
    cursor->policy_size = 8;
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
    app_init((App *) eddy);
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
