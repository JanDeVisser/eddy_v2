/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef APP_MODE_H
#define APP_MODE_H

#include <app/widget.h>
#include <base/lexer.h>

#define _MODE_FIELDS \
    _WIDGET_FIELDS;  \
    Language *language;

typedef struct _mode {
    _MODE_FIELDS;
} Mode;

#define _M               \
    union {              \
        Mode _mode;      \
        struct {         \
            _MODE_FIELDS \
        };               \
    }

WIDGET_CLASS(Mode, mode);
#define MODE_CLASS(c, prefix)          \
    extern void    prefix##_init(c *); \
    WidgetHandlers $##c##_handlers

#define MODE_CLASS_DEF(c, prefix)           \
    WidgetHandlers $##c##_handlers = {      \
        .init = (WidgetInit) prefix##_init, \
    }

#endif /* APP_MODE_H */
