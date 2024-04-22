/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef APP_MODE_H
#define APP_MODE_H

#include <app/widget.h>
#include <base/lexer.h>
#include <lsp/lsp.h>

#define MODE_FIELDS                    \
    _WIDGET_FIELDS;                    \
    StringList    filetypes;           \
    WidgetFactory data_widget_factory; \
    Language     *language;            \
    LSP           lsp;

typedef struct mode {
    MODE_FIELDS;
} Mode;

#define MODE            \
    union {             \
        Mode mode;      \
        struct {        \
            MODE_FIELDS \
        };              \
    }

WIDGET_CLASS(Mode, mode);
#define MODE_CLASS(c, prefix)          \
    extern void    prefix##_init(c *); \
    WidgetHandlers $##c##_handlers

#define MODE_CLASS_DEF(c, prefix)           \
    WidgetHandlers $##c##_handlers = {      \
        .init = (WidgetInit) prefix##_init, \
    }

#define MODE_DATA_FIELDS \
    _WIDGET_FIELDS;      \
    Mode *mode;

typedef struct mode_data {
    MODE_DATA_FIELDS;
} ModeData;

#define MODE_DATA            \
    union {                  \
        ModeData mode_data;  \
        struct {             \
            MODE_DATA_FIELDS \
        };                   \
    }

WIDGET_CLASS(ModeData, mode_data);
#define MODE_DATA_CLASS(c, prefix)     \
    extern void    prefix##_init(c *); \
    WidgetHandlers $##c##_handlers

#define MODE_DATA_CLASS_DEF(c, prefix)      \
    WidgetHandlers $##c##_handlers = {      \
        .init = (WidgetInit) prefix##_init, \
    }

extern void      mode_cmd_split_line(Mode *mode, JSONValue unused);
extern void      mode_cmd_indent(Mode *mode, JSONValue unused);
extern void      mode_cmd_unindent(Mode *mode, JSONValue unused);
extern bool      mode_character(Mode *mode, int ch);
extern ModeData *mode_make_data(Mode *mode);

#endif /* APP_MODE_H */
