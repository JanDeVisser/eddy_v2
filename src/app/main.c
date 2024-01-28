/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <math.h>
#include <raylib.h>

#define STATIC_ALLOCATOR
#include <allocate.h>
#include <io.h>

#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 768

#define DEBUG_PANE_WIDTH 400
#define DEBUG_PANE_HEIGHT WINDOW_HEIGHT

#define PADDING 5.0f

typedef enum {
    KMOD_NONE = 0,
    KMOD_SHIFT,
    KMOD_CONTROL,
    KMOD_ALT,
    KMOD_SUPER,
} KeyboardModifier;

typedef struct {
    union {
        int column;
        int x;
    };
    union {
        int line;
        int y;
    };
} IntVector2;

typedef struct {
    union {
        int column;
        int x;
    };
    union {
        int line;
        int y;
    };
    int w;
    int h;
} IntRectangle;

typedef struct _widget {
    Rectangle viewport;
} Widget;

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
    Rectangle viewport;
} Gutter;

typedef struct {
    Rectangle  viewport;
    Buffers    buffers;
    IntVector2 outline;
    int        current_buffer;
    double     cursor_flash;
    int        columns;
    int        lines;
} Editor;

typedef struct {
    Rectangle viewport;
    char      last_key[64];
} DebugPane;

typedef struct {
    Rectangle viewport;
    Gutter    gutter;
    Editor    editor;
    int       lines;
} MainArea;

typedef struct {
    Rectangle viewport;
    int       argc;
    char    **argv;
    MainArea  main;
    DebugPane debug_pane;
    Font      font;
    Vector2   cell;
    double    time;
} App;

App app = { 0 };

int imin(int i1, int i2)
{
    return (i2 < i1) ? i2 : i1;
}

bool is_key_pressed(int key, KeyboardModifier modifier, char const *keystr, char const *modstr)
{
    KeyboardModifier current_modifier = KMOD_NONE;
    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
        current_modifier |= KMOD_SHIFT;
    }
    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
        current_modifier |= KMOD_CONTROL;
    }
    if (IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER)) {
        current_modifier |= KMOD_SUPER;
    }
    if (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) {
        current_modifier |= KMOD_ALT;
    }
    if ((IsKeyPressed(key) || IsKeyPressedRepeat(key)) && modifier == current_modifier) {
        char const *s = modifier != KMOD_NONE ? TextFormat("%s | %s", keystr, modstr) : keystr;
        strcpy(app.debug_pane.last_key, s);
        return true;
    }
    return false;
}

#define IS_PRESSED(key,mod) (is_key_pressed((key), (mod), #key, #mod))

void widget_render_text(void *w, float x, float y, StringView text, Color color)
{
    Widget *widget = (Widget *) w;
    char    ch = text.ptr[text.length];
    ((char *) text.ptr)[text.length] = 0;
    Vector2 pos = { widget->viewport.x + PADDING + x, widget->viewport.y + PADDING + y };
    DrawTextEx(app.font, text.ptr, pos, app.font.baseSize, 2, color);
    ((char *) text.ptr)[text.length] = ch;
}

void widget_render_text_bitmap(void *w, float x, float y, StringView text, Color color)
{
    Widget *widget = (Widget *) w;
    char    ch = text.ptr[text.length];
    ((char *) text.ptr)[text.length] = 0;
    DrawText(text.ptr, widget->viewport.x + PADDING + x, widget->viewport.y + PADDING + y, 20, color);
    ((char *) text.ptr)[text.length] = ch;
}

void widget_draw_rectangle(void *w, float x, float y, float width, float height, Color color)
{
    Widget *widget = (Widget *) w;
    DrawRectangle(widget->viewport.x + x, widget->viewport.y + y, width, height, color);
}

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

void gutter_init(Gutter *)
{
}

void gutter_resize(Gutter *gutter)
{
    gutter->viewport.x = 0;
    gutter->viewport.y = 0;
    gutter->viewport.width = 4 * app.cell.x + 2 * PADDING;
    gutter->viewport.height = app.viewport.height;
}

void gutter_draw(Gutter *gutter)
{
    Buffer *buffer = app.main.editor.buffers.elements + app.main.editor.current_buffer;
    for (int row = 0; row < app.main.lines && buffer->top_line + row < buffer->lines.size; ++row) {
        size_t lineno = buffer->top_line + row;
        widget_render_text(gutter, 0, app.cell.y * row, sv_from(TextFormat("%4d", lineno + 1)), BEIGE);
    }
}

void gutter_handle(Gutter *)
{
}

extern void editor_init(Editor *editor);
extern void editor_open_buffer(Editor *editor, StringView file);
extern void editor_new(Editor *editor);

void editor_init(Editor *editor)
{
    for (int ix = 1; ix < app.argc; ++ix) {
        editor_open_buffer(editor, sv_from(app.argv[ix]));
    }
    if (editor->buffers.size == 0) {
        editor_new(editor);
    }
}

void editor_resize(Editor *editor)
{
    editor->viewport.x = app.main.gutter.viewport.width;
    editor->viewport.y = 0;
    editor->viewport.width = app.main.viewport.width - app.main.gutter.viewport.width;
    editor->viewport.height = app.viewport.height;
    editor->cursor_flash = GetTime();
    editor->columns = (int) ((editor->viewport.width - 2 * PADDING) / app.cell.x);
    editor->lines = app.main.lines;
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
        widget_render_text(editor, 0, app.cell.y * row, (StringView) { line.line.ptr, ix }, RAYWHITE);
    }

    double time = app.time - editor->cursor_flash;
    if (time - floor(time) < 0.5) {
        int x = buffer->cursor_pos.x - buffer->left_column;
        int y = buffer->cursor_pos.y - buffer->top_line;
        widget_draw_rectangle(editor, 5.0f + x * app.cell.x, 5.0f + y * app.cell.y, 2, app.cell.y, RAYWHITE);
    }
}

void editor_handle(Editor *editor)
{
    Buffer *buffer = app.main.editor.buffers.elements + app.main.editor.current_buffer;
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
        sb_remove(&buffer->text, line.index_of + line.line.length - 1, 1);
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
        app.main.editor.cursor_flash = app.time;
    }
}

void debug_init(DebugPane *debug)
{
}

void debug_resize(DebugPane *debug)
{
    debug->viewport.x = app.viewport.width - DEBUG_PANE_WIDTH;
    debug->viewport.y = 0;
    debug->viewport.width = DEBUG_PANE_WIDTH;
    debug->viewport.height = app.viewport.height;
}

void debug_draw(DebugPane *debug)
{
    Buffer *buffer = app.main.editor.buffers.elements + app.main.editor.current_buffer;
    size_t  y = 0;
    widget_render_text_bitmap(debug, 0, y, sv_from(TextFormat("Window: %d Columns %d Rows", app.main.editor.columns, app.main.editor.lines)), WHITE);
    y += 22;
    widget_render_text_bitmap(debug, 0, y, sv_from(TextFormat("Lines: %zu", buffer->lines.size)), WHITE);
    y += 22;
    widget_render_text_bitmap(debug, 0, y, sv_from(TextFormat("Bytes: %zu", buffer->text.view.length)), WHITE);
    y += 22;
    widget_render_text_bitmap(debug, 0, y, sv_from(TextFormat("Cursor: Index %zu Col %d (%d) Row %d ", buffer->cursor, buffer->cursor_pos.x, buffer->cursor_col, buffer->cursor_pos.y)), WHITE);
    y += 22;
    Index *index = buffer->lines.elements + buffer->cursor_pos.line;
    widget_render_text_bitmap(debug, 0, y, sv_from(TextFormat("Current Line: Index %zu Length %zu", index->index_of, index->line.length)), WHITE);
    y += 22;
    widget_render_text_bitmap(debug, 0, y, sv_from(TextFormat("Text viewport: Col %zu Row %zu", buffer->left_column, buffer->top_line)), WHITE);
    y += 22;
    widget_render_text_bitmap(debug, 0, y, sv_from(TextFormat("Last Key: %s", app.debug_pane.last_key)), WHITE);
    y += 22;
    DrawFPS(app.debug_pane.viewport.x + PADDING, y + PADDING);
    y += 22;
}

void debug_handle(DebugPane *)
{
}

void mainarea_init(MainArea *main)
{
    gutter_init(&main->gutter);
    editor_init(&main->editor);
}

void mainarea_resize(MainArea *main)
{
    main->viewport.x = 0;
    main->viewport.y = 0;
    main->viewport.width = app.viewport.width - app.debug_pane.viewport.width;
    main->viewport.height = app.viewport.height;
    main->lines = (int) ((main->viewport.height - 2 * PADDING) / app.cell.y);
    gutter_resize(&main->gutter);
    editor_resize(&main->editor);
}

void mainarea_draw(MainArea *main)
{
    gutter_draw(&main->gutter);
    editor_draw(&main->editor);
}

void mainarea_handle(MainArea *main)
{
    gutter_handle(&main->gutter);
    editor_handle(&main->editor);
}

void app_resize(App *app)
{
    app->viewport.x = 0;
    app->viewport.y = 0;
    app->viewport.width = GetScreenWidth();
    app->viewport.height = GetScreenHeight();
    Vector2 measurements = MeasureTextEx(app->font, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", (float) app->font.baseSize, 2);
    app->cell.x = measurements.x / 52.0f;
    int rows = (app->viewport.height - 10) / measurements.y;
    app->cell.y = (float) (app->viewport.height - 10) / (float) rows;
    debug_resize(&app->debug_pane);
    mainarea_resize(&app->main);
}

void app_init(App *app, int argc, char **argv)
{
    app->argc = argc;
    app->argv = argv;
    app->font = LoadFontEx("fonts/VictorMono-Medium.ttf", 30, 0, 250);
    debug_init(&app->debug_pane);
    mainarea_init(&app->main);

    app_resize(app);
}

void app_draw(App *app)
{
    mainarea_draw(&app->main);
    debug_draw(&app->debug_pane);
}

void app_handle(App *app)
{
    app->time = GetTime();
    if (IsWindowResized()) {
        app_resize(app);
    }
    mainarea_handle(&app->main);
    debug_handle(&app->debug_pane);
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

    app_init(&app, argc, argv);

    while (!WindowShouldClose()) {

        if (IsKeyPressed(KEY_Q) && (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))) {
            break;
        }

        app_handle(&app);

        BeginDrawing();
        ClearBackground(DARKGRAY);
        app_draw(&app);
        EndDrawing();
    }
    UnloadFont(app.font);
    CloseWindow();

    return 0;
}
