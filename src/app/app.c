/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <widget.h>

DA_IMPL(PendingCommand);
DA_IMPL(DrawFloating);

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

void app_draw_floating(App *app, void *target, WidgetDraw draw)
{
    assert(target != NULL);
    assert(draw != NULL);
    da_append_DrawFloating(&app->floatings, (DrawFloating) { .target = (Widget *) target, .draw = draw });
}

void app_draw(App *app)
{
    app->floatings.size = 0;
    layout_draw((Layout *) app);
    for (size_t ix = 0; ix < app->floatings.size; ++ix) {
        DrawFloating *floating = app->floatings.elements + ix;
        floating->draw(floating->target);
    }
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

    app->commands_mutex = mutex_create();
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
            CommandBinding *b = w->bindings.elements + bix;
            int             key = b->key_combo.key;
            if ((IsKeyPressed(key) || IsKeyPressedRepeat(key)) && b->key_combo.modifier == modifier) {
                JSONValue key_combo = json_object();
                json_set(&key_combo, "key", json_int(key));
                json_set(&key_combo, "modifier", json_int(modifier));
                widget_command_execute(w, b->command, key_combo);
                json_free(key_combo);
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

void app_submit(App *app, void *target, StringView command, JSONValue args)
{
    mutex_lock(app->commands_mutex);
    trace(CAT_EDIT, "app_submit: Pushing command '%.*s' for widget of class '%s'", SV_ARG(command), ((Widget *) target)->classname);
    da_append_PendingCommand(
        &app->pending_commands,
        (PendingCommand) {
            .target = (Widget *) target,
            .command = sv_copy(command),
            .arguments = args,
        });
    mutex_unlock(app->commands_mutex);
}

void app_process_input(App *app)
{
    if (app->pending_commands.size > 0) {
        mutex_lock(app->commands_mutex);
        PendingCommand pending = da_pop_front_PendingCommand(&app->pending_commands);
        mutex_unlock(app->commands_mutex);
        trace(CAT_EDIT, "app_process_input: Popped command '%.*s' for widget of class '%s'", SV_ARG(pending.command), pending.target->classname);
        widget_command_execute(pending.target, pending.command, pending.arguments);
        sv_free(pending.command);
        json_free(pending.arguments);
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
