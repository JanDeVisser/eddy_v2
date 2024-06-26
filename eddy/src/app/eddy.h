/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef APP_EDDY_H
#define APP_EDDY_H

#include <app/buffer.h>
#include <app/editor.h>
#include <app/mode.h>
#include <app/theme.h>
#include <app/widget.h>
#include <lsp/lsp.h>

typedef enum {
    AS_MONITOR = 0,
    AS_COUNT,
} AppStateItem;

typedef struct {
    int state[AS_COUNT];
} AppState;

typedef struct {
    _L;
} StatusBar;

LAYOUT_CLASS(StatusBar, sb);

typedef struct {
    StringView cmakelists;
    StringView build_dir;
} CMake;

typedef struct {
    _A;
    Buffers    buffers;
    Editor    *editor;
    StringView project_dir;
    StringList source_dirs;
    CMake      cmake;
    JSONValue  settings;
    Theme      theme;
    Widgets    modes;
} Eddy;

APP_CLASS(Eddy, eddy);

extern void app_state_read(AppState *state);
extern void app_state_write(AppState *state);

extern void          eddy_process_input(Eddy *eddy);
extern void          eddy_on_draw(Eddy *eddy);
extern void          eddy_on_start(Eddy *eddy);
extern void          eddy_on_terminate(Eddy *eddy);
extern void          eddy_open_dir(Eddy *eddy, StringView dir);
extern ErrorOrBuffer eddy_open_buffer(Eddy *e, StringView file);
extern Buffer       *eddy_new_buffer(Eddy *e);
extern void          eddy_close_buffer(Eddy *eddy, int buffer_num);
extern void          eddy_set_message(Eddy *eddy, char const *fmt, ...);
extern void          eddy_clear_message(Eddy *eddy);
extern void          eddy_load_font(Eddy *eddy);
void                 eddy_inc_font_size(Eddy *e, int increment);
extern Eddy         *eddy_create();
extern Mode         *eddy_get_mode_for_buffer(Eddy *e, StringView buffer_name);

extern AppState app_state;
extern Eddy     eddy;

#endif /* APP_EDDY_H */
