/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <math.h>
#include <raylib.h>

#include <allocate.h>

#include <eddy.h>
#include <palette.h>
#include <widget.h>

#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 768

DECLARE_SHARED_ALLOCATOR(eddy);
SHARED_ALLOCATOR_IMPL(eddy);

int main(int argc, char **argv)
{
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "eddy");
    SetWindowMonitor(0);
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

        app_process_input((App *) &eddy);

        BeginDrawing();
        ClearBackground(palettes[PALETTE_DARK][PI_BACKGROUND]);
        layout_draw((Layout *) &eddy);
        EndDrawing();
    }
    UnloadFont(eddy.font);
    CloseWindow();

    return 0;
}
