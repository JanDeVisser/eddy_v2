/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __APP_WIDGET_H__
#define __APP_WIDGET_H__

#include <raylib.h>

#include <sv.h>

#define PADDING 5.0f

#define VALUETYPES(S) \
    S(STRING)         \
    S(INT)

typedef enum {
#undef VALUETYPE
#define VALUETYPE(T) VT_##T,
    VALUETYPES(VALUETYPE)
#undef VALUETYPE
} ValueType;

typedef struct {
    ValueType type;
    union {
        StringView string;
        int        intval;
    };
} Value;

DA_WITH_NAME(Value, Values);

#define KEYBOARDMODIFIERS(S) \
    S(NONE, 0, "")           \
    S(SHIFT, 1, "S-")        \
    S(CONTROL, 2, "C-")      \
    S(ALT, 4, "M-")          \
    S(SUPER, 8, "U-")

typedef enum {
#undef KEYBOARDMODIFIER
#define KEYBOARDMODIFIER(mod, ord, str) KMOD_##mod = ord,
    KEYBOARDMODIFIERS(KEYBOARDMODIFIER)
#undef KEYBOARDMODIFIER
        KMOD_COUNT,
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
typedef void (*WidgetOnDraw)(Widget *);
typedef void (*WidgetDraw)(Widget *);
typedef void (*WidgetAfterDraw)(Widget *);
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
    WidgetOnDraw            on_draw;
    WidgetDraw              draw;
    WidgetAfterDraw         after_draw;
} WidgetHandlers;

typedef struct {
    int              key;
    KeyboardModifier modifier;
} KeyCombo;

typedef struct {
    KeyCombo   trigger;
    StringView called_as;
    Widget    *target;
    Values     arguments;
} CommandContext;

typedef void (*CommandHandler)(CommandContext *);

typedef struct {
    StringView     name;
    CommandHandler handler;
} Command;

DA_WITH_NAME(Command, Commands);

typedef struct {
    KeyCombo key_combo;
    size_t   command;
} CommandBinding;

DA_WITH_NAME(CommandBinding, CommandBindings);

#define _WIDGET_FIELDS           \
    char const     *classname;   \
    WidgetHandlers  handlers;    \
    Rect            viewport;    \
    Widget         *parent;      \
    SizePolicy      policy;      \
    float           policy_size; \
    Commands        commands;    \
    CommandBindings bindings;    \
    Widget         *memo;

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

#define WIDGET_CLASS(c, prefix)                        \
    extern void           prefix##_init(c *);          \
    extern void           prefix##_resize(c *);        \
    extern void           prefix##_process_input(c *); \
    extern void           prefix##_draw(c *);          \
    extern WidgetHandlers $##c##_handlers;

#define WIDGET_CLASS_DEF(c, prefix)                           \
    WidgetHandlers $##c##_handlers = {                        \
        .init = (WidgetInit) prefix##_init,                   \
        .resize = (WidgetResize) prefix##_resize,             \
        .process_input = (WidgetDraw) prefix##_process_input, \
        .draw = (WidgetDraw) prefix##_draw,                   \
    }

#define SIMPLE_WIDGET_CLASS(c, prefix)        \
    extern void           prefix##_init(c *); \
    extern WidgetHandlers $##c##_handlers;

#define SIMPLE_WIDGET_CLASS_DEF(c, prefix)  \
    WidgetHandlers $##c##_handlers = {      \
        .init = (WidgetInit) prefix##_init, \
    }

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
    }

WIDGET_CLASS(Layout, layout);
#define LAYOUT_CLASS(c, prefix)        \
    extern void    prefix##_init(c *); \
    WidgetHandlers $##c##_handlers

#define LAYOUT_CLASS_DEF(c, prefix)                         \
    WidgetHandlers $##c##_handlers = {                      \
        .init = (WidgetInit) prefix##_init,                 \
        .resize = (WidgetResize) layout_resize,             \
        .process_input = (WidgetDraw) layout_process_input, \
        .draw = (WidgetDraw) layout_draw,                   \
    }

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

#define _APP_FIELDS      \
    _LAYOUT_FIELDS;      \
    int     argc;        \
    char  **argv;        \
    Widget *focus;       \
    Font    font;        \
    int     monitor;     \
    Vector2 cell;        \
    double  time;        \
    size_t  frame_count; \
    char    last_key[64]

typedef struct {
    _APP_FIELDS;
} App;

#define _A               \
    union {              \
        App _app;        \
        struct {         \
            _APP_FIELDS; \
        };               \
    };

LAYOUT_CLASS(App, app);
#define APP_CLASS(c, prefix)           \
    extern void    prefix##_init(c *); \
    WidgetHandlers $##c##_handlers

#define APP_CLASS_DEF(c, prefix)                                 \
    WidgetHandlers $##c##_handlers = {                           \
        .init = (WidgetInit) prefix##_init,                      \
        .resize = (WidgetResize) layout_resize,                  \
        .process_input = (WidgetProcessInput) app_process_input, \
        .draw = (WidgetDraw) layout_draw,                        \
    }

extern int         iclamp(int v, int min, int max);
extern int         imin(int i1, int i2);
extern int         imax(int i1, int i2);
extern bool        icontains(int f, int min, int max);
extern char const *rect_tostring(Rect r);
extern bool        is_modifier_down(KeyboardModifier modifier);
extern char const *modifier_string(KeyboardModifier modifier);
extern bool        _is_key_pressed(int key, char const *keystr, ...);
extern void        widget_render_text(void *w, float x, float y, StringView text, Color color);
extern void        widget_render_text_bitmap(void *w, float x, float y, StringView text, Color color);
extern void        widget_draw_rectangle(void *w, float x, float y, float width, float height, Color color);
extern void        _widget_add_command(void *w, StringView cmd, CommandHandler handler, ...);
extern Widget     *layout_find_by_draw_function(Layout *layout, WidgetDraw draw_fnc);
extern void        layout_add_widget(Layout *layout, Widget *widget);
extern void        layout_traverse(Layout *layout, void (*fnc)(Widget *));
extern void        layout_dump(Layout *layout);
extern void        app_initialize(App *app, WidgetInit init, int argc, char **argv);
extern void        app_process_input(App *app);
extern void        app_on_resize(App *app);
extern void        app_on_process_input(App *app);

#define is_key_pressed(key, ...) (_is_key_pressed((key), #key __VA_OPT__(, ) __VA_ARGS__, KMOD_COUNT))
#define widget_add_command(w, cmd, handler, ...) _widget_add_command((w), (cmd), (handler) __VA_OPT__(, ) __VA_ARGS__, (KeyCombo) { KEY_NULL, KMOD_NONE })

extern App *app;

#endif /* __APP_WIDGET_H__ */
