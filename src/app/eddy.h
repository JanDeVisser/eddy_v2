/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __APP_EDDY_H__
#define __APP_EDDY_H__

#include <allocate.h>

#include <buffer.h>
#include <editor.h>
#include <widget.h>

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
    _W;
    StringView message;
    double     time;
} MessageLine;

WIDGET_CLASS(MessageLine, message_line);

typedef struct {
    _A;
    Buffers    buffers;
    Editor    *editor;
    StringView project_dir;
} Eddy;

APP_CLASS(Eddy, eddy);

extern void app_state_read(AppState *state);
extern void app_state_write(AppState *state);

extern void          eddy_process_input(Eddy *eddy);
extern void          eddy_on_draw(Eddy *eddy);
extern void          eddy_on_start(Eddy *eddy);
extern void          eddy_on_terminate(Eddy *eddy);
extern void          eddy_open_dir(Eddy *eddy, StringView dir);
extern ErrorOrBuffer eddy_open_buffer(Eddy *eddy, StringView file);
extern Buffer       *eddy_new_buffer(Eddy *eddy);
extern void          eddy_set_message(Eddy *eddy, StringView message);
extern void          eddy_clear_message(Eddy *eddy);
extern Eddy         *eddy_create();

extern AppState state;
extern Eddy     eddy;

#include <widget.h>

#endif /* __APP_EDDY_H__ */
