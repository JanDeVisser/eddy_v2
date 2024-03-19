/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __APP_EDITOR_H__
#define __APP_EDITOR_H__

#include "sv.h"
#include <widget.h>

typedef struct {
    _W int     buffer_num;
    size_t     cursor;
    IntVector2 cursor_pos;
    int        cursor_col;
    size_t     new_cursor;
    int        top_line;
    int        left_column;
    size_t     selection;
    int        cursor_flash;
    StringView find_text;
    Widget    *mode;
} BufferView;

DA_WITH_NAME(BufferView, BufferViews);
SIMPLE_WIDGET_CLASS(BufferView, view);

typedef struct {
    _W;
    BufferViews buffers;
    int         current_buffer;
    int         columns;
    int         lines;
    double      clicks[3];
    int         num_clicks;
} Editor;

WIDGET_CLASS(Editor, editor);

typedef struct {
    _W;
} Gutter;

WIDGET_CLASS(Gutter, gutter);
extern void       editor_new(Editor *editor);
extern ErrorOrInt editor_open(Editor *editor, StringView file);
extern void       editor_select_buffer(Editor *editor, int buffer_num);
extern void       editor_close_view(Editor *editor);
extern void       editor_close_buffer(Editor *editor);
extern bool       editor_has_prev(Editor *editor);
extern bool       editor_has_next(Editor *editor);
extern void       editor_select_prev(Editor *editor);
extern void       editor_select_next(Editor *editor);
extern void       editor_update_cursor(Editor *editor);
extern void       editor_insert(Editor *editor, StringView text, size_t at);
extern void       editor_insert_string(Editor *editor, StringView sv);
extern bool       editor_character(Editor *editor, int ch);
extern void       editor_delete(Editor *editor, size_t at, size_t count);
extern void       editor_lines_up(Editor *editor, int count);
extern void       editor_lines_down(Editor *editor, int count);
extern void       editor_manage_selection(Editor *editor, BufferView *view, bool selection);
extern void       editor_selection_to_clipboard(Editor *editor);
extern void       editor_select_line(Editor *editor);

#endif /* __APP_EDITOR_H__ */
