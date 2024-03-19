/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __APP_MINIBUFFER_H__
#define __APP_MINIBUFFER_H__

#include <app/widget.h>
#include <stdarg.h>

typedef void (*MiniBufferQueryFunction)(void *w, StringView query);

typedef struct {
    _W void                *target;
    StringView              prompt;
    StringBuilder           text;
    size_t                  cursor;
    MiniBufferQueryFunction fnc;
} MiniBufferQuery;

WIDGET_CLASS(MiniBufferQuery, mb_query);

typedef struct {
    _W;
    StringView      message;
    double          time;
    MiniBufferQuery current_query;
} MiniBuffer;

WIDGET_CLASS(MiniBuffer, minibuffer);

extern void minibuffer_set_vmessage(char const *fmt, va_list args);
extern void minibuffer_set_message(char const *fmt, ...);
extern void minibuffer_clear_message();
extern void minibuffer_query(void *w, StringView prompt, MiniBufferQueryFunction fnc);

#endif /* __APP_MINIBUFFER_H__ */