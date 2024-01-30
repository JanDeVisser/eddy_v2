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

typedef enum {
    CO_VERTICAL = 0,
    CO_HORIZONTAL,
} ContainerOrientation;

typedef enum {
    SP_ABSOLUTE = 0,
    SP_RELATIVE,
    SP_CHARACTERS,
    SP_CALCULATED,
    SP_STRETCH,
} SizePolicy;

typedef union {
    struct {
        float x;
        float y;
        float width;
        float height;
    };
    Rectangle r;
    float     coords[4];
    struct {
        float position[2];
        float size[2];
    };
} Rect;

DA_WITH_NAME(Rect, Rects);
DA_IMPL(Rect);

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

typedef struct _widget Widget;

DA_STRUCT_WITH_NAME(Widget, Widget *, Widgets);

typedef void (*WidgetInit)(Widget *);
typedef void (*WidgetDraw)(Widget *);
typedef void (*WidgetOnResize)(Widget *);
typedef void (*WidgetResize)(Widget *);
typedef void (*WidgetAfterResize)(Widget *);
typedef void (*WidgetOnProcessInput)(Widget *);
typedef void (*WidgetProcessInput)(Widget *);
typedef void (*WidgetAfterProcessInput)(Widget *);

typedef struct {
    WidgetInit              init;
    WidgetOnResize          on_resize;
    WidgetResize            resize;
    WidgetAfterResize       after_resize;
    WidgetOnProcessInput    on_process_input;
    WidgetProcessInput      process_input;
    WidgetAfterProcessInput after_process_input;
    WidgetDraw              draw;
} WidgetHandlers;

#define _WIDGET_FIELDS        \
    char const    *classname; \
    WidgetHandlers handlers;  \
    Rect           viewport;  \
    Widget        *parent;    \
    SizePolicy     policy;    \
    float          policy_size;

typedef struct _widget {
    _WIDGET_FIELDS
} Widget;

#define _W                 \
    union {                \
        Widget _widget;    \
        struct {           \
            _WIDGET_FIELDS \
        };                 \
    };

#define widget_init(w) ((((Widget *) (w))->handlers.init)((w)))
#define widget_on_resize(w)             \
    ({                                  \
        Widget *_w = (Widget *) (w);    \
        if (_w->handlers.on_resize)     \
            _w->handlers.on_resize(_w); \
    })
#define widget_resize(w)             \
    ({                               \
        Widget *_w = (Widget *) (w); \
        if (_w->handlers.resize)     \
            _w->handlers.resize(_w); \
    })
#define widget_after_resize(w)             \
    ({                                     \
        Widget *_w = (Widget *) (w);       \
        if (_w->handlers.after_resize)     \
            _w->handlers.after_resize(_w); \
    })
#define widget_draw(w)               \
    ({                               \
        Widget *_w = (Widget *) (w); \
        if (_w->handlers.draw)       \
            _w->handlers.draw(_w);   \
    })
#define widget_on_process_input(w)             \
    ({                                         \
        Widget *_w = (Widget *) (w);           \
        if (_w->handlers.on_process_input)     \
            _w->handlers.on_process_input(_w); \
    })
#define widget_process_input(w)             \
    ({                                      \
        Widget *_w = (Widget *) (w);        \
        if (_w->handlers.process_input)     \
            _w->handlers.process_input(_w); \
    })
#define widget_after_process_input(w)             \
    ({                                            \
        Widget *_w = (Widget *) (w);              \
        if (_w->handlers.after_process_input)     \
            _w->handlers.after_process_input(_w); \
    })

#define WIDGET_CLASS(c, prefix)                               \
    extern void    prefix##_init(c *);                        \
    extern void    prefix##_resize(c *);                      \
    extern void    prefix##_process_input(c *);               \
    extern void    prefix##_draw(c *);                        \
    WidgetHandlers $##c##_handlers = {                        \
        .init = (WidgetInit) prefix##_init,                   \
        .resize = (WidgetResize) prefix##_resize,             \
        .process_input = (WidgetDraw) prefix##_process_input, \
        .draw = (WidgetDraw) prefix##_draw,                   \
    };

#define SIMPLE_WIDGET_CLASS(c, prefix)      \
    extern void    prefix##_init(c *);      \
    WidgetHandlers $##c##_handlers = {      \
        .init = (WidgetInit) prefix##_init, \
    };

#define widget_new(c)                            \
    ({                                           \
        Widget *_w = (Widget *) allocate_new(c); \
        _w->classname = #c;                      \
        _w->handlers = $##c##_handlers;          \
        _w->handlers.init(_w);                   \
        _w;                                      \
    })

#define widget_new_with_policy(c, p, s)          \
    ({                                           \
        Widget *_w = (Widget *) allocate_new(c); \
        _w->classname = #c;                      \
        _w->handlers = $##c##_handlers;          \
        _w->handlers.init(_w);                   \
        _w->policy = (p);                        \
        _w->policy_size = (s);                   \
        _w;                                      \
    })

#define _LAYOUT_FIELDS                \
    _WIDGET_FIELDS;                   \
    ContainerOrientation orientation; \
    Widgets              widgets;

typedef struct _layout {
    _LAYOUT_FIELDS;
} Layout;

#define _L                 \
    union {                \
        Layout _layout;    \
        struct {           \
            _LAYOUT_FIELDS \
        };                 \
    };

WIDGET_CLASS(Layout, layout);
#define LAYOUT_CLASS(c, prefix)                             \
    extern void    prefix##_init(c *);                      \
    WidgetHandlers $##c##_handlers = {                      \
        .init = (WidgetInit) prefix##_init,                 \
        .resize = (WidgetResize) layout_resize,             \
        .process_input = (WidgetDraw) layout_process_input, \
        .draw = (WidgetDraw) layout_draw,                   \
    };
extern Widget *layout_find_by_draw_function(Layout *layout, WidgetDraw draw_fnc);

#define layout_new(o)                               \
    ({                                              \
        Layout *_l = (Layout *) widget_new(Layout); \
        _l->orientation = (o);                      \
        _l;                                         \
    })

typedef struct {
    _W;
} Spacer;

SIMPLE_WIDGET_CLASS(Spacer, spacer);

typedef struct {
    _W;
    Color      color;
    StringView text;
} Label;

WIDGET_CLASS(Label, label);

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

LAYOUT_CLASS(StatusBar, status_bar);

typedef struct {
    _W;
    StringView message;
} MessageLine;

WIDGET_CLASS(MessageLine, message_line);

typedef struct {
    _L;
    int     argc;
    char  **argv;
    Font    font;
    Vector2 cell;
    double  time;
    Editor *editor;
    char    last_key[64];
} App;

LAYOUT_CLASS(App, app);
extern void app_initialize(App *app, int argc, char **argv);
extern void app_on_resize(App *app);
extern void app_on_process_input(App *app);
extern void app_set_message(App *app, StringView message);
extern void app_clear_message(App *app);

App app = { 0 };

DA_IMPL_TYPE(Widget, Widget *);

int imin(int i1, int i2)
{
    return (i2 < i1) ? i2 : i1;
}

char const *rect_tostring(Rect r)
{
    return TextFormat("%fx%f@+%f,+%f", r.width, r.height, r.x, r.y);
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
    if (current_modifier != modifier) {
        return false;
    }
    if (IsKeyPressed(key) || IsKeyPressedRepeat(key)) {
        char const *s = modifier != KMOD_NONE ? TextFormat("%s | %s", keystr, modstr) : keystr;
        strcpy(app.last_key, s);
        return true;
    }
    return false;
}

#define IS_PRESSED(key, mod) (is_key_pressed((key), (mod), #key, #mod))

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

void layout_init(Layout *)
{
}

void layout_add_widget(Layout *layout, Widget *widget)
{
    da_append_Widget(&layout->widgets, widget);
    widget->parent = (Widget *) layout;
}

void layout_resize(Layout *layout)
{
    printf("Resizing layout %s %s\n", layout->classname, rect_tostring(layout->viewport));
    widget_on_resize(layout);
    float allocated = 0.0f;
    int   stretch_count = 0;
    int   fixed_coord = (layout->orientation == CO_VERTICAL) ? 0 : 1;
    int   var_coord = 1 - fixed_coord;
    float total = layout->viewport.size[var_coord];
    float fixed_size = layout->viewport.size[fixed_coord];
    float fixed_pos = layout->viewport.position[fixed_coord];
    float var_offset = layout->viewport.position[var_coord];

    printf("Total available %f, laying out %s\n", total, (layout->orientation == CO_VERTICAL) ? "vertically" : "horizontally");
    printf("Fixed %s: %f, fixed %s position: %f\n",
        (layout->orientation == CO_VERTICAL) ? "width" : "height",
        fixed_size,
        (layout->orientation == CO_VERTICAL) ? "x" : "y",
        fixed_pos);
    for (size_t ix = 0; ix < layout->widgets.size; ++ix) {
        Widget *w = layout->widgets.elements[ix];
        w->viewport.size[fixed_coord] = fixed_size;
        w->viewport.position[fixed_coord] = fixed_pos;
        float sz = 0;
        printf("Component widget %s has policy %d\n", w->classname, w->policy);
        switch (w->policy) {
        case SP_ABSOLUTE:
            sz = w->policy_size;
            break;
        case SP_RELATIVE: {
            sz = (total * w->policy_size) / 100.0f;
        } break;
        case SP_CHARACTERS: {
            sz = w->policy_size * ((layout->orientation == CO_VERTICAL) ? app.cell.y : app.cell.x) + 2 * PADDING;
        } break;
        case SP_CALCULATED: {
            NYI("SP_CALCULATED not yet supported");
        } break;
        case SP_STRETCH: {
            sz = -1.0f;
            stretch_count++;
        } break;
        }
        assert_msg(sz != 0, "Size Policy %d resulted in zero size", (int) w->policy);
        w->viewport.size[var_coord] = sz;
        if (sz > 0) {
            allocated += sz;
            printf("Allocating %f, now allocated %f\n", sz, allocated);
        }
    }

    if (stretch_count) {
        printf("Stretch count %d\n", stretch_count);
        assert_msg(total > allocated, "No room left in container for %d stretched components. Available: %f Allocated: %f", stretch_count, total, allocated);
        float stretch = (total - allocated) / (float) stretch_count;
        for (size_t ix = 0; ix < layout->widgets.size; ++ix) {
            Widget *w = layout->widgets.elements[ix];
            if (w->policy == SP_STRETCH) {
                printf("Allocating %f to stretchable %s\n", stretch, w->classname);
                w->viewport.size[var_coord] = stretch;
            }
        }
    }

    for (size_t ix = 0; ix < layout->widgets.size; ++ix) {
        Widget *w = layout->widgets.elements[ix];
        w->viewport.position[var_coord] = var_offset;
        var_offset += w->viewport.size[var_coord];
        printf("Resizing %s to %s\n", w->classname, rect_tostring(w->viewport));
        widget_resize(w);
    }
    widget_after_resize(layout);
}

void layout_draw(Layout *layout)
{
    for (size_t ix = 0; ix < layout->widgets.size; ++ix) {
        Widget *w = layout->widgets.elements[ix];
        if (w->viewport.width > 0.0f && w->viewport.height > 0.0f) {
            widget_draw(layout->widgets.elements[ix]);
        }
    }
}

void layout_process_input(Layout *layout)
{
    widget_on_process_input(layout);
    for (size_t ix = 0; ix < layout->widgets.size; ++ix) {
        Widget *w = layout->widgets.elements[ix];
        if (w->viewport.width > 0.0f && w->viewport.height > 0.0f) {
            widget_process_input(layout->widgets.elements[ix]);
        }
    }
    widget_after_process_input(layout);
}

Widget *layout_find_by_draw_function(Layout *layout, WidgetDraw draw_fnc)
{
    for (size_t ix = 0; ix < layout->widgets.size; ++ix) {
        Widget *w = (Widget *) layout->widgets.elements[ix];
        if (w->handlers.draw == draw_fnc) {
            return w;
        }
        if (w->handlers.resize == (WidgetResize) layout_resize) {
            Widget *ret = layout_find_by_draw_function((Layout *) w, draw_fnc);
            if (ret) {
                return ret;
            }
        }
    }
    return NULL;
}

void spacer_init(Spacer *spacer)
{
    spacer->policy = SP_STRETCH;
}

void label_init(Label *label)
{
    label->policy = SP_CHARACTERS;
    label->color = RAYWHITE;
}

void label_resize(Label *label)
{
}

void label_draw(Label *label)
{
    if (!sv_empty(label->text)) {
        widget_render_text(label, 0, 0, label->text, label->color);
    }
}

void label_process_input(Label *)
{
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
    Buffer *buffer = app.editor->buffers.elements + app.editor->current_buffer;
    for (int row = 0; row < app.editor->lines && buffer->top_line + row < buffer->lines.size; ++row) {
        size_t lineno = buffer->top_line + row;
        widget_render_text(gutter, 0, app.cell.y * row, sv_from(TextFormat("%4d", lineno + 1)), BEIGE);
    }
}

void gutter_process_input(Gutter *)
{
}

void editor_init(Editor *editor)
{
    editor->policy = SP_STRETCH;
    for (int ix = 1; ix < app.argc; ++ix) {
        editor_open_buffer(editor, sv_from(app.argv[ix]));
    }
    if (editor->buffers.size == 0) {
        editor_new(editor);
    }
}

void editor_resize(Editor *editor)
{
    editor->cursor_flash = GetTime();
    editor->columns = (int) ((editor->viewport.width - 2 * PADDING) / app.cell.x);
    editor->lines = (int) ((editor->viewport.height - 2 * PADDING) / app.cell.y);
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
        editor->cursor_flash = app.time;
    }
}

void sb_file_name_draw(Label *label)
{
    Buffer *buffer = app.editor->buffers.elements + app.editor->current_buffer;
    label->text = buffer->name;
    label_draw(label);
}

void sb_cursor_draw(Label *label)
{
    Buffer *buffer = app.editor->buffers.elements + app.editor->current_buffer;
    label->text = sv_from(TextFormat("%4d:%d", buffer->cursor_pos.line + 1, buffer->cursor_pos.column + 1));
    label_draw(label);
}

void sb_last_key_draw(Label *label)
{
    label->text = sv_from(app.last_key);
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

void status_bar_init(StatusBar *status_bar)
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

void app_on_resize(App *app)
{
    Vector2 measurements = MeasureTextEx(app->font, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", (float) app->font.baseSize, 2);
    app->cell.x = measurements.x / 52.0f;
    int rows = (app->viewport.height - 10) / measurements.y;
    app->cell.y = (float) (app->viewport.height - 10) / (float) rows;
    app->viewport = (Rect) { 0 };
    app->viewport.width = GetScreenWidth();
    app->viewport.height = GetScreenHeight();
}

void app_initialize(App *app, int argc, char **argv)
{
    app->argc = argc;
    app->argv = argv;
    app->handlers = $App_handlers;
    app->classname = "App";
    app_init(app);
}

void app_init(App *app)
{
    app->font = LoadFontEx("fonts/VictorMono-Medium.ttf", 30, 0, 250);
    app->handlers.on_resize = (WidgetOnResize) app_on_resize;
    app->handlers.on_process_input = (WidgetOnProcessInput) app_on_process_input;
    app->orientation = CO_HORIZONTAL;

    Layout *editor_pane = layout_new(CO_HORIZONTAL);
    editor_pane->policy = SP_STRETCH;
    layout_add_widget(editor_pane, widget_new(Gutter));
    layout_add_widget(editor_pane, widget_new(Editor));

    Layout *main_area = layout_new(CO_VERTICAL);
    main_area->policy = SP_STRETCH;
    layout_add_widget(main_area, (Widget *) editor_pane);
    layout_add_widget(main_area, widget_new(StatusBar));
    layout_add_widget(main_area, widget_new(MessageLine));

    layout_add_widget((Layout *) app, (Widget *) main_area);
    app->editor = (Editor *) layout_find_by_draw_function((Layout *) app, (WidgetDraw) editor_draw);
    app->viewport = (Rect) { 0 };
    app->viewport.width = GetScreenWidth();
    app->viewport.height = GetScreenHeight();
    layout_resize((Layout *) app);
}

void app_on_process_input(App *app)
{
    app->time = GetTime();
    if (IsWindowResized()) {
        Rect r = { 0 };
        app->viewport.width = GetScreenWidth();
        app->viewport.height = GetScreenHeight();
        layout_resize((Layout *) app);
    }
}

void app_set_message(App *app, StringView message)
{
    MessageLine *message_line = (MessageLine *) layout_find_by_draw_function((Layout *) app, (WidgetDraw) message_line_draw);
    assert(message_line);
    if (!sv_empty(message_line->message)) {
        sv_free(message_line->message);
    }
    message_line->message = message;
}

void app_clear_message(App *app)
{
    MessageLine *message_line = (MessageLine *) layout_find_by_draw_function((Layout *) app, (WidgetDraw) message_line_draw);
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

    app_initialize(&app, argc, argv);

    while (!WindowShouldClose()) {

        if (IsKeyPressed(KEY_Q) && (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))) {
            break;
        }

        layout_process_input((Layout *) &app);

        BeginDrawing();
        ClearBackground(DARKGRAY);
        layout_draw((Layout *) &app);
        EndDrawing();
    }
    UnloadFont(app.font);
    CloseWindow();

    return 0;
}
