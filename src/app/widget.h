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
    CO_HORIZONTAL = 0,
    CO_VERTICAL,
} ContainerOrientation;

typedef enum {
    SP_ABSOLUTE = 0,
    SP_RELATIVE,
    SP_CHARACTERS,
    SP_CALCULATED,
    SP_STRETCH,
} SizePolicy;

typedef enum {
    ModalStatusDormant = 0,
    ModalStatusActive,
    ModalStatusSubmitted,
    ModalStatusDismissed,
} ModalStatus;

typedef union {
    struct {
        float x;
        float y;
        float width;
        float height;
    };
    struct {
        float left;
        float top;
        float right;
        float bottom;
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
typedef void (*WidgetOnStart)(Widget *);
typedef void (*WidgetOnDraw)(Widget *);
typedef void (*WidgetDraw)(Widget *);
typedef void (*WidgetAfterDraw)(Widget *);
typedef void (*WidgetOnResize)(Widget *);
typedef void (*WidgetResize)(Widget *);
typedef void (*WidgetAfterResize)(Widget *);
typedef bool (*WidgetHandleCharacter)(Widget *, int);
typedef void (*WidgetOnProcessInput)(Widget *);
typedef void (*WidgetProcessInput)(Widget *);
typedef void (*WidgetAfterProcessInput)(Widget *);
typedef void (*WidgetOnTerminate)(Widget *);

typedef struct {
    WidgetInit              init;
    WidgetOnStart           on_start;
    WidgetOnResize          on_resize;
    WidgetResize            resize;
    WidgetAfterResize       after_resize;
    WidgetHandleCharacter   character;
    WidgetOnProcessInput    on_process_input;
    WidgetProcessInput      process_input;
    WidgetAfterProcessInput after_process_input;
    WidgetOnDraw            on_draw;
    WidgetDraw              draw;
    WidgetAfterDraw         after_draw;
    WidgetOnTerminate       on_terminate;
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
    Widget        *target;
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
    Rect            padding;     \
    Widget         *parent;      \
    SizePolicy      policy;      \
    float           policy_size; \
    Color           background;  \
    Commands        commands;    \
    CommandBindings bindings;    \
    void           *memo;

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

#define widget_new(C)                      \
    (C *) ({                               \
        Widget *_w = (Widget *) MALLOC(C); \
        _w->classname = #C;                \
        _w->handlers = $##C##_handlers;    \
        _w->handlers.init(_w);             \
        _w;                                \
    })

#define widget_with_init(C, I)              \
    (C *) ({                                \
        Widget *_w = (Widget *) MALLOC(C);  \
        _w->classname = #C;                 \
        _w->handlers = $##C##_handlers;     \
        _w->handlers.init = (WidgetInit) I; \
        I((C *) _w);                        \
        _w;                                 \
    })

#define in_place_widget(C, W, P)        \
    (C *) ({                            \
        Widget *_w = (Widget *) (W);    \
        _w->classname = #C;             \
        _w->handlers = $##C##_handlers; \
        _w->parent = (Widget *) (P);    \
        _w->handlers.init(_w);          \
        (W);                            \
    })

#define in_place_widget_with_init(C, W, P, I) \
    (C *) ({                                  \
        Widget *_w = (Widget *) (W);          \
        _w->classname = #C;                   \
        _w->handlers = $##C##_handlers;       \
        _w->handlers.init = (WidgetInit) I;   \
        _w->parent = (Widget *) (P);          \
        I((C *) _w);                          \
        (W);                                  \
    })

#define widget_new_with_parent(C, P)       \
    (C *) ({                               \
        Widget *_w = (Widget *) MALLOC(C); \
        _w->classname = #C;                \
        _w->handlers = $##C##_handlers;    \
        _w->parent = (Widget *) (P);       \
        _w->handlers.init(_w);             \
        _w;                                \
    })

#define widget_new_with_policy(C, P, S)    \
    (C *) ({                               \
        Widget *_w = (Widget *) MALLOC(C); \
        _w->classname = #C;                \
        _w->handlers = $##C##_handlers;    \
        _w->handlers.init(_w);             \
        _w->policy = (P);                  \
        _w->policy_size = (S);             \
        _w;                                \
    })

#define DEFAULT_PADDING ((Rect) { .left = PADDING, .top = PADDING, .right = PADDING, .bottom = PADDING })

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

#define _APP_FIELDS            \
    _LAYOUT_FIELDS;            \
    int      argc;             \
    char   **argv;             \
    int      monitor;          \
    Font     font;             \
    Widget  *focus;            \
    Vector2  cell;             \
    Ints     queue;            \
    char     last_key[64];     \
    bool     quit;             \
    double   time;             \
    Widgets  modals;           \
    Commands pending_commands; \
    size_t   frame_count

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

typedef App *(*AppCreate)(void);
typedef bool (*LayoutFindByPredicate)(Layout *layout, Widget *w, void *ctx);

extern char const *SizePolicy_name(SizePolicy policy);
extern int         iclamp(int v, int min, int max);
extern int         imin(int i1, int i2);
extern int         imax(int i1, int i2);
extern bool        icontains(int f, int min, int max);
extern float       fclamp(float v, float min, float max);
extern bool        fcontains(float f, float min, float max);
extern char const *rect_tostring(Rect r);
extern bool        is_modifier_down(KeyboardModifier modifier);
extern char const *modifier_string(KeyboardModifier modifier);
extern bool        _is_key_pressed(int key, char const *keystr, ...);
extern Rectangle   widget_normalize(void *w, float left, float top, float width, float height);
extern void        widget_render_text(void *w, float x, float y, StringView text, Font font, Color color);
extern void        widget_render_sized_text(void *w, float x, float y, StringView text, Font font, float size, Color color);
extern void        widget_render_text_bitmap(void *w, float x, float y, StringView text, Color color);
extern void        widget_draw_rectangle(void *w, float x, float y, float width, float height, Color color);
extern void        widget_draw_outline(void *w, float x, float y, float width, float height, Color color);
extern void        widget_draw_line(void *w, float x0, float y0, float x1, float y1, Color color);
extern void        _widget_add_command(void *w, StringView cmd, CommandHandler handler, ...);
extern bool        widget_contains(void *widget, Vector2 world_coordinates);
extern Widget     *layout_find_by_predicate(Layout *layout, LayoutFindByPredicate predicate, void *ctx);
extern Widget     *layout_find_by_draw_function(Layout *layout, WidgetDraw draw_fnc);
extern Widget     *layout_find_by_classname(Layout *layout, StringView classname);
extern void        layout_add_widget(Layout *layout, void *widget);
extern void        layout_traverse(Layout *layout, void (*fnc)(Widget *));
extern void        layout_dump(Layout *layout);
extern void        app_initialize(AppCreate create, int argc, char **argv);
extern void        app_start();
extern void        app_init(App *app);
extern void        app_draw(App *app);
extern void        app_process_input(App *app);
extern void        app_on_resize(App *app);
extern void        app_on_process_input(App *app);

#define is_key_pressed(key, ...) (_is_key_pressed((key), #key __VA_OPT__(, ) __VA_ARGS__, KMOD_COUNT))
#define widget_add_command(w, cmd, handler, ...) _widget_add_command((w), (cmd), (handler) __VA_OPT__(, ) __VA_ARGS__, (KeyCombo) { KEY_NULL, KMOD_NONE })

extern App *app;

#endif /* __APP_WIDGET_H__ */
