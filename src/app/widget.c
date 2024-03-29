/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <math.h>

#include <allocate.h>
#include <widget.h>

DECLARE_SHARED_ALLOCATOR(eddy)
DA_IMPL(Rect);
DA_IMPL(Command);
DA_IMPL(CommandBinding);
DA_IMPL_TYPE(Widget, Widget *);

WIDGET_CLASS_DEF(Layout, layout);
SIMPLE_WIDGET_CLASS_DEF(Spacer, spacer);
SIMPLE_WIDGET_CLASS_DEF(Label, label);
LAYOUT_CLASS_DEF(App, app);

App *app;

char const *SizePolicy_name(SizePolicy policy)
{
    switch (policy) {
    case SP_ABSOLUTE:
        return "Absolute";
    case SP_RELATIVE:
        return "Relative";
    case SP_CHARACTERS:
        return "Characters";
    case SP_CALCULATED:
        return "Calculated";
    case SP_STRETCH:
        return "Stretch";
    default:
        UNREACHABLE();
    }
}

int iclamp(int v, int min, int max)
{
    return imin(imax(v, min), max);
}

int imin(int i1, int i2)
{
    return (i2 < i1) ? i2 : i1;
}

int imax(int i1, int i2)
{
    return (i2 > i1) ? i2 : i1;
}

bool icontains(int v, int min, int max)
{
    return v >= min && v <= max;
}

float fclamp(float v, float min, float max)
{
    return fmin(fmax(v, min), max);
}

bool fcontains(float v, float min, float max)
{
    return v >= min && v <= max;
}

char const *rect_tostring(Rect r)
{
    return TextFormat("%.1fx%f.1@+%.1f,+%.1f", r.width, r.height, r.x, r.y);
}

KeyboardModifier modifier_current()
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
    return current_modifier;
}

bool is_modifier_down(KeyboardModifier modifier)
{
    KeyboardModifier current_modifier = modifier_current();
    if (modifier == KMOD_NONE) {
        return current_modifier == KMOD_NONE;
    }
    return (current_modifier & modifier) == modifier;
}

char const *modifier_string(KeyboardModifier modifier)
{
    static char buffer[64];
    buffer[0] = 0;
#undef KEYBOARDMODIFIER
#define KEYBOARDMODIFIER(mod, ord, str) \
    if (KMOD_##mod & modifier)          \
        strcat(buffer, str);
    KEYBOARDMODIFIERS(KEYBOARDMODIFIER)
#undef KEYBOARDMODIFIER
    return buffer;
}

bool _is_key_pressed(int key, char const *keystr, ...)
{
    if (!IsKeyPressed(key) && !IsKeyPressedRepeat(key)) {
        return false;
    }
    va_list mods;
    va_start(mods, keystr);
    KeyboardModifier current = modifier_current();

    for (KeyboardModifier mod = va_arg(mods, KeyboardModifier); mod != KMOD_COUNT; mod = va_arg(mods, KeyboardModifier)) {
        if (mod == current) {
            strcpy(app->last_key, TextFormat("%s%s", modifier_string(mod), keystr));
            va_end(mods);
            return true;
        }
    }
    va_end(mods);
    return false;
}

Rectangle widget_normalize(void *w, float left, float top, float width, float height)
{
    Widget *widget = (Widget *) w;
    if (left < 0) {
        left = widget->viewport.width + left;
    }
    if (top < 0) {
        top = widget->viewport.height + top;
    }
    if (width <= 0) {
        width = widget->viewport.width + 2 * width;
    }
    if (height <= 0) {
        height = widget->viewport.height + 2 * height;
    }
    left = fclamp(left, 0, widget->viewport.width);
    top = fclamp(top, 0, widget->viewport.height);
    width = fclamp(width, 0, widget->viewport.width - 1);
    height = fclamp(height, 0, widget->viewport.height - 1);
    return (Rectangle) { .x = widget->viewport.x + left, .y = widget->viewport.y + top, .width = width, .height = height };
}

void widget_render_text(void *w, float x, float y, StringView text, Font font, Color color)
{
    widget_render_sized_text(w, x, y, text, font, 1.0, color);
}

void widget_render_sized_text(void *w, float x, float y, StringView text, Font font, float size, Color color)
{
    Widget *widget = (Widget *) w;
    if (sv_empty(text)) {
        return;
    }
    char ch = text.ptr[text.length];
    if (ch) {
        ((char *) text.ptr)[text.length] = 0;
    }
    if (x < 0 || y < 0) {
        Vector2 m = MeasureTextEx(font, text.ptr, /* font.baseSize */ 20.0 * size, 2);
        if (x < 0) {
            x = widget->viewport.width - m.x + x;
        }
        if (y < 0) {
            y = widget->viewport.height - m.y + y;
        }
    }
    Vector2 pos = { widget->viewport.x + x, widget->viewport.y + y };
    DrawTextEx(font, text.ptr, pos, /* font.baseSize */ 20.0 * size, 2, color);
    if (ch) {
        ((char *) text.ptr)[text.length] = ch;
    }
}

void widget_render_text_bitmap(void *w, float x, float y, StringView text, Color color)
{
    Widget *widget = (Widget *) w;
    char    ch = text.ptr[text.length];
    if (ch != 0) {
        ((char *) text.ptr)[text.length] = 0;
    }
    if (x < 0) {
        int text_width = MeasureText(text.ptr, 20);
        x = widget->viewport.width - text_width + x;
    }
    if (y < 0) {
        y = widget->viewport.height - 20 + y;
    }
    DrawText(text.ptr, widget->viewport.x + PADDING + x, widget->viewport.y + PADDING + y, 20, color);
    if (ch != 0) {
        ((char *) text.ptr)[text.length] = ch;
    }
}

void widget_draw_line(void *w, float x0, float y0, float x1, float y1, Color color)
{
    Widget *widget = (Widget *) w;
    if (x0 < 0) {
        x0 = widget->viewport.width + x0;
    }
    if (y0 < 0) {
        y0 = widget->viewport.height + y0;
    }
    if (x1 <= 0) {
        x1 = widget->viewport.width + x1;
    }
    if (y1 <= 0) {
        y1 = widget->viewport.height + y1;
    }
    x0 = fclamp(x0, 0, widget->viewport.width);
    y0 = fclamp(y0, 0, widget->viewport.height);
    x1 = fclamp(x1, 0, widget->viewport.width - 1);
    y1 = fclamp(y1, 0, widget->viewport.height - 1);
    DrawLine(widget->viewport.x + x0, widget->viewport.y + y0,
        widget->viewport.x + x1, widget->viewport.y + y1,
        color);
}

void widget_draw_rectangle(void *w, float x, float y, float width, float height, Color color)
{
    Widget *widget = (Widget *) w;
    DrawRectangleRec(widget_normalize(w, x, y, width, height), color);
}

void widget_draw_outline(void *w, float x, float y, float width, float height, Color color)
{
    Widget *widget = (Widget *) w;
    DrawRectangleLinesEx(widget_normalize(w, x, y, width, height), 1, color);
}

void _widget_add_command(void *w, StringView cmd, CommandHandler handler, ...)
{
    Widget *widget = (Widget *) w;
    da_append_Command(&widget->commands, (Command) { w, cmd, handler });
    size_t  ix = widget->commands.size - 1;
    va_list bindings;
    va_start(bindings, handler);
    for (KeyCombo key_combo = va_arg(bindings, KeyCombo); key_combo.key != KEY_NULL; key_combo = va_arg(bindings, KeyCombo)) {
        da_append_CommandBinding(&widget->bindings, (CommandBinding) { key_combo, ix });
    }
    va_end(bindings);
}

bool widget_contains(void *widget, Vector2 world_coordinates)
{
    Widget *w = (Widget *) widget;
    Rect    r = w->viewport;
    return (r.x < world_coordinates.x) && (world_coordinates.x < r.x + r.width) && (r.y < world_coordinates.y) && (world_coordinates.y < r.y + r.height);
}

void layout_init(Layout *)
{
}

void layout_add_widget(Layout *layout, void *widget)
{
    Widget *w = (Widget *) widget;
    da_append_Widget(&layout->widgets, w);
    w->parent = (Widget *) layout;
}

void layout_resize(Layout *layout)
{
    // printf("Resizing layout %s %s\n", layout->classname, rect_tostring(layout->viewport));
    Widget *w = (Widget *) layout;
    if (w->handlers.on_resize) {
        w->handlers.on_resize(w);
    }
    float allocated = 0.0f;
    int   stretch_count = 0;
    int   fixed_coord = (layout->orientation == CO_VERTICAL) ? 0 : 1;
    int   var_coord = 1 - fixed_coord;
    float total = layout->viewport.size[var_coord];
    float fixed_size = layout->viewport.size[fixed_coord];
    float fixed_pos = layout->viewport.position[fixed_coord];
    float var_offset = layout->viewport.position[var_coord];

    // printf("Total available %f, laying out %s\n", total, (layout->orientation == CO_VERTICAL) ? "vertically" : "horizontally");
    // printf("Fixed %s: %f, fixed %s position: %f\n",
    //     (layout->orientation == CO_VERTICAL) ? "width" : "height",
    //     fixed_size,
    //     (layout->orientation == CO_VERTICAL) ? "x" : "y",
    //     fixed_pos);
    for (size_t ix = 0; ix < layout->widgets.size; ++ix) {
        w = layout->widgets.elements[ix];
        w->viewport.size[fixed_coord] = fixed_size - w->padding.coords[fixed_coord] - w->padding.coords[fixed_coord + 2];
        w->viewport.position[fixed_coord] = fixed_pos + w->padding.coords[fixed_coord];
        float sz = 0;
        // printf("Component widget %s has policy %s\n", w->classname, SizePolicy_name(w->policy));
        switch (w->policy) {
        case SP_ABSOLUTE:
            sz = w->policy_size;
            break;
        case SP_RELATIVE: {
            sz = (total * w->policy_size) / 100.0f;
        } break;
        case SP_CHARACTERS: {
            sz = ceilf(1.2 * w->policy_size * ((layout->orientation == CO_VERTICAL) ? app->cell.y : app->cell.x));
        } break;
        case SP_CALCULATED: {
            NYI("SP_CALCULATED not yet supported");
        };
        case SP_STRETCH: {
            sz = -1.0f;
            stretch_count++;
        } break;
        }
        assert_msg(sz != 0, "Size Policy %s resulted in zero size", SizePolicy_name(w->policy));
        w->viewport.size[var_coord] = sz - w->padding.coords[var_coord] - w->padding.coords[var_coord + 2];
        if (sz > 0) {
            allocated += sz;
            // printf("Allocating %f, now allocated %f\n", sz, allocated);
        }
    }

    if (stretch_count) {
        // printf("Stretch count %d\n", stretch_count);
        assert_msg(total > allocated, "No room left in container for %d stretched components. Available: %f Allocated: %f", stretch_count, total, allocated);
        float stretch = floorf((total - allocated) / (float) stretch_count);
        for (size_t ix = 0; ix < layout->widgets.size; ++ix) {
            w = layout->widgets.elements[ix];
            if (w->policy == SP_STRETCH) {
                // printf("Allocating %f to stretchable %s\n", stretch, w->classname);
                w->viewport.size[var_coord] = stretch - w->padding.coords[var_coord] - w->padding.coords[var_coord + 2];
            }
        }
    }

    for (size_t ix = 0; ix < layout->widgets.size; ++ix) {
        w = layout->widgets.elements[ix];
        w->viewport.position[var_coord] = var_offset + w->padding.coords[var_coord];
        var_offset += w->viewport.size[var_coord] + w->padding.coords[var_coord] + w->padding.coords[var_coord + 2];
        // printf("Resizing %s to %s\n", w->classname, rect_tostring(w->viewport));
        if (w->handlers.resize) {
            w->handlers.resize(w);
        }
    }
    if (layout->handlers.after_resize) {
        layout->handlers.after_resize((Widget *) layout);
    }
}

void layout_draw(Layout *layout)
{
    Widget *w = (Widget *) layout;
    if (w->handlers.on_draw) {
        w->handlers.on_draw(w);
    }
    for (size_t ix = 0; ix < layout->widgets.size; ++ix) {
        w = layout->widgets.elements[ix];
        if (w->viewport.width > 0.0f && w->viewport.height > 0.0f && w->handlers.draw) {
            DrawRectangle(w->viewport.x - w->padding.left, w->viewport.y - w->padding.top,
                w->viewport.width + w->padding.left + w->padding.right,
                w->viewport.height + w->padding.top + w->padding.bottom,
                w->background);
            w->handlers.draw(w);
        }
    }
    if (layout->handlers.after_draw) {
        layout->handlers.after_draw((Widget *) layout);
    }
}

void layout_process_input(Layout *layout)
{
    Widget *w = (Widget *) layout;
    if (w->handlers.on_process_input) {
        w->handlers.on_process_input(w);
    }
    for (size_t ix = 0; ix < layout->widgets.size; ++ix) {
        w = layout->widgets.elements[ix];
        if (w->viewport.width > 0.0f && w->viewport.height > 0.0f && w->handlers.process_input) {
            w->handlers.process_input(w);
        }
    }
    if (layout->handlers.after_process_input) {
        layout->handlers.after_process_input((Widget *) layout);
    }
}

Widget *layout_find_by_predicate(Layout *layout, LayoutFindByPredicate predicate, void *ctx)
{
    for (size_t ix = 0; ix < layout->widgets.size; ++ix) {
        Widget *w = (Widget *) layout->widgets.elements[ix];
        if (predicate(layout, w, ctx)) {
            return w;
        }
        if (w->handlers.resize == (WidgetResize) layout_resize) {
            Widget *ret = layout_find_by_predicate((Layout *) w, predicate, ctx);
            if (ret) {
                return ret;
            }
        }
    }
    return NULL;
}

bool find_by_draw_fnc(Layout *layout, Widget *widget, void *ctx)
{
    return (widget->handlers.draw == (WidgetDraw) ctx);
}

Widget *layout_find_by_draw_function(Layout *layout, WidgetDraw draw_fnc)
{
    return layout_find_by_predicate(layout, find_by_draw_fnc, draw_fnc);
}

bool find_by_classname(Layout *layout, Widget *widget, void *ctx)
{
    return sv_eq_cstr(*(StringView *) ctx, widget->classname);
}

Widget *layout_find_by_classname(Layout *layout, StringView classname)
{
    return layout_find_by_predicate(layout, find_by_classname, &classname);
}

void layout_traverse(Layout *layout, void (*fnc)(Widget *))
{
    fnc((Widget *) layout);
    for (size_t ix = 0; ix < layout->widgets.size; ++ix) {
        Widget *w = layout->widgets.elements[ix];
        if (w->handlers.resize == (WidgetResize) layout_resize) {
            layout_traverse((Layout *) w, fnc);
        } else {
            fnc(w);
        }
    }
    fnc(NULL);
}

static int layout_dump_indent = 0;

void _layout_dump_fnc(Widget *w)
{
    if (w == NULL) {
        layout_dump_indent -= 2;
        return;
    }
    if (w->handlers.resize == (WidgetResize) layout_resize) {
        Layout *l = (Layout *) w;
        printf("%*s+ | %zu %s\n", layout_dump_indent, "", l->widgets.size, rect_tostring(w->viewport));
        layout_dump_indent += 2;
        return;
    }
    printf("%*s+-> %s %s\n", layout_dump_indent, "", w->classname, rect_tostring(w->viewport));
}

void layout_dump(Layout *layout)
{
    layout_dump_indent = 0;
    layout_traverse(layout, _layout_dump_fnc);
}

void spacer_init(Spacer *spacer)
{
    spacer->policy = SP_STRETCH;
}

void label_init(Label *label)
{
    label->policy = SP_CHARACTERS;
    label->policy_size = 1.0f;
    label->color = RAYWHITE;
    label->padding = DEFAULT_PADDING;
}

void label_resize(Label *label)
{
}

void label_draw(Label *label)
{
    if (!sv_empty(label->text)) {
        widget_render_text(label, 0, 0, label->text, app->font, label->color);
    }
}

void label_process_input(Label *)
{
}

void app_init(App *app)
{
    if (!app->handlers.resize) {
        app->handlers.resize = (WidgetResize) layout_resize;
    }
    if (!app->handlers.process_input) {
        app->handlers.process_input = (WidgetProcessInput) app_process_input;
    }
    if (!app->handlers.draw) {
        app->handlers.draw = (WidgetDraw) app_draw;
    }
    if (!app->handlers.on_resize) {
        app->handlers.on_resize = (WidgetOnResize) app_on_resize;
    }
    if (!app->handlers.on_process_input) {
        app->handlers.on_process_input = (WidgetOnProcessInput) app_on_process_input;
    }
}

void app_draw(App *app)
{
    layout_draw((Layout *) app);
    for (size_t ix = 0; ix < app->modals.size; ++ix) {
        Widget *m = app->modals.elements[ix];
        m->handlers.draw(m);
    }
}

void app_on_resize(App *app)
{
    Vector2 measurements = MeasureTextEx(app->font, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz",
        /* (float) app->font.baseSize */ 20.0, 2);
    app->cell.x = measurements.x / 52.0f;
    int rows = (app->viewport.height - 10) / measurements.y;
    app->cell.y = (float) (app->viewport.height - 10) / (float) rows;
    app->viewport = (Rect) { 0 };
    app->viewport.width = GetScreenWidth();
    app->viewport.height = GetScreenHeight();
}

void app_initialize(AppCreate create, int argc, char **argv)
{
    app = create();
    app->argc = argc;
    app->argv = argv;
    if (!app->classname) {
        app->classname = "App";
    }
    if (!app->handlers.init) {
        app->handlers.init = (WidgetInit) app_init;
    }

    app->handlers.init((Widget *) app);

    InitWindow(app->viewport.width, app->viewport.height, app->classname);
    app->viewport.width = GetScreenWidth();
    app->viewport.height = GetScreenHeight();
    SetWindowMonitor(app->monitor);
    SetWindowState(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_MAXIMIZED | FLAG_VSYNC_HINT);
    Image icon = LoadImage("eddy.png");
    SetWindowIcon(icon);
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    MaximizeWindow();
}

void app_start()
{
    if (app->handlers.on_start) {
        app->handlers.on_start((Widget *) app);
    }
    layout_resize((Layout *) app);
    while (!WindowShouldClose() && !app->quit) {
        app->handlers.process_input((Widget *) app);
        BeginDrawing();
        app->handlers.draw((Widget *) app);
        EndDrawing();
    }
    if (app->handlers.on_terminate) {
        app->handlers.on_terminate((Widget *) app);
    }
    CloseWindow();
}

void app_on_process_input(App *app)
{
    app->time = GetTime();
    ++app->frame_count;
    if (IsWindowResized()) {
        Rect r = { 0 };
        app->viewport.width = GetScreenWidth();
        app->viewport.height = GetScreenHeight();
        layout_resize((Layout *) app);
    }
    if (GetCurrentMonitor() != app->monitor) {
        app->monitor = GetCurrentMonitor();
    }
}

bool find_and_run_shortcut(Widget *w, KeyboardModifier modifier)
{
    for (; w; w = w->parent) {
        for (size_t bix = 0; bix < w->bindings.size; ++bix) {
            int key = w->bindings.elements[bix].key_combo.key;
            if ((IsKeyPressed(key) || IsKeyPressedRepeat(key)) && w->bindings.elements[bix].key_combo.modifier == modifier) {
                assert(w->bindings.elements[bix].command < w->commands.size);
                Command       *command = w->commands.elements + w->bindings.elements[bix].command;
                CommandContext ctx = { 0 };
                ctx.trigger = w->bindings.elements[bix].key_combo;
                ctx.called_as = command->name;
                ctx.target = w;
                command->handler(&ctx);
                return true;
            }
        }
    }
    return false;
}

void handle_characters(App *app, Widget *w)
{
    while (app->queue.size > 0) {
        int ch = app->queue.elements[0];
        for (; w != NULL; w = w->parent) {
            if (w->handlers.character != NULL) {
                if (w->handlers.character(w, ch)) {
                    break;
                }
            }
        }
        da_pop_front_int(&app->queue);
    }
}

void app_process_input(App *app)
{
    if (app->pending_commands.size > 0) {
        // Command command = *da_element_Command(&app->pending_commands, 0);
        // memmove(app->pending_commands.elements, app->pending_commands.elements + 1, sizeof(Command) * app->pending_commands.size);
        // --app->pending_commands.size;
        Command        command = da_pop_front_Command(&app->pending_commands);
        CommandContext ctx = { 0 };
        ctx.trigger = (KeyCombo) { .key = KEY_NULL, KMOD_NONE };
        ctx.called_as = command.name;
        ctx.target = command.target;
        command.handler(&ctx);
        return;
    }
    for (int ch = GetCharPressed(); ch != 0; ch = GetCharPressed()) {
        da_append_int(&app->queue, ch);
    }
    if (app->modals.size) {
        Widget *modal = app->modals.elements[app->modals.size - 1];
        handle_characters(app, modal);
        modal->handlers.process_input(modal);
        return;
    }
    Widget *f = app->focus;
    if (!f) {
        f = (Widget *) app;
    }
    KeyboardModifier modifier = modifier_current();
    if (!find_and_run_shortcut(f, modifier)) {
        handle_characters(app, f);
        layout_process_input((Layout *) app);
    }
}
