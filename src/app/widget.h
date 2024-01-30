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

#define _APP_FIELDS \
    _LAYOUT_FIELDS; \
    int     argc;   \
    char  **argv;   \
    Font    font;   \
    Vector2 cell;   \
    double  time;   \
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

#define APP_CLASS_DEF(c, prefix)                            \
    WidgetHandlers $##c##_handlers = {                      \
        .init = (WidgetInit) prefix##_init,                 \
        .resize = (WidgetResize) layout_resize,             \
        .process_input = (WidgetDraw) layout_process_input, \
        .draw = (WidgetDraw) layout_draw,                   \
    }

extern int         imin(int i1, int i2);
extern char const *rect_tostring(Rect r);
extern bool        is_key_pressed(int key, KeyboardModifier modifier, char const *keystr, char const *modstr);
extern void        widget_render_text(void *w, float x, float y, StringView text, Color color);
extern void        widget_render_text_bitmap(void *w, float x, float y, StringView text, Color color);
extern void        widget_draw_rectangle(void *w, float x, float y, float width, float height, Color color);
extern Widget     *layout_find_by_draw_function(Layout *layout, WidgetDraw draw_fnc);
extern void        layout_add_widget(Layout *layout, Widget *widget);
extern void        app_initialize(App *app, WidgetInit init, int argc, char **argv);
extern void        app_on_resize(App *app);
extern void        app_on_process_input(App *app);

#define IS_PRESSED(key, mod) (is_key_pressed((key), (mod), #key, #mod))

extern App *app;

#endif /* __APP_WIDGET_H__ */
