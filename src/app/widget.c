/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <allocate.h>
#include <widget.h>

DECLARE_SHARED_ALLOCATOR(eddy)
DA_IMPL(Rect);
DA_IMPL_TYPE(Widget, Widget *);

WIDGET_CLASS_DEF(Layout, layout);
SIMPLE_WIDGET_CLASS_DEF(Spacer, spacer);
SIMPLE_WIDGET_CLASS_DEF(Label, label);
LAYOUT_CLASS_DEF(App, app);

App *app;

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
        strcpy(app->last_key, s);
        return true;
    }
    return false;
}

void widget_render_text(void *w, float x, float y, StringView text, Color color)
{
    Widget *widget = (Widget *) w;
    char    ch = text.ptr[text.length];
    ((char *) text.ptr)[text.length] = 0;
    Vector2 pos = { widget->viewport.x + PADDING + x, widget->viewport.y + PADDING + y };
    DrawTextEx(app->font, text.ptr, pos, app->font.baseSize, 2, color);
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
            sz = w->policy_size * ((layout->orientation == CO_VERTICAL) ? app->cell.y : app->cell.x) + 2 * PADDING;
        } break;
        case SP_CALCULATED: {
            NYI("SP_CALCULATED not yet supported");
        };
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

void app_init(App *app)
{
    app->font = LoadFontEx("fonts/VictorMono-Medium.ttf", 30, 0, 250);
    app->handlers.on_resize = (WidgetOnResize) app_on_resize;
    app->handlers.on_process_input = (WidgetOnProcessInput) app_on_process_input;
    app->orientation = CO_HORIZONTAL;
    app->viewport = (Rect) { 0 };
    app->viewport.width = GetScreenWidth();
    app->viewport.height = GetScreenHeight();
    layout_resize((Layout *) app);
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

void app_initialize(App *the_app, WidgetInit init, int argc, char **argv)
{
    app = the_app;
    app->argc = argc;
    app->argv = argv;
    app->handlers = $App_handlers;
    app->handlers.init = init;
    app->classname = "App";
    init((Widget*) app);
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
