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

typedef struct {
    _L;
} StatusBar;

LAYOUT_CLASS(StatusBar, sb);

typedef struct {
    _W;
    StringView message;
} MessageLine;

WIDGET_CLASS(MessageLine, message_line);

typedef struct {
    _A;
    Buffers buffers;
    Editor *editor;
} Eddy;

APP_CLASS(Eddy, eddy);

extern void eddy_on_draw(Eddy *eddy);
extern void eddy_open_buffer(Eddy *eddy, StringView file);
extern void eddy_set_message(Eddy *eddy, StringView message);
extern void eddy_clear_message(Eddy *eddy);

extern Eddy eddy;

#include <widget.h>

#endif /* __APP_EDDY_H__ */
