/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __APP_BUFFER_H__
#define __APP_BUFFER_H__

#include <sv.h>

typedef struct {
    size_t     index_of;
    StringView line;
} Index;

DA_WITH_NAME(Index, Indices);

typedef struct {
    StringView    name;
    StringBuilder text;
    Indices       lines;
    bool          rebuild_needed;
    bool          dirty;
} Buffer;

DA_WITH_NAME(Buffer, Buffers);
ERROR_OR_ALIAS(Buffer, Buffer *);

extern ErrorOrBuffer buffer_open(Buffer *buffer, StringView name);
extern Buffer       *buffer_new(Buffer *buffer);
extern size_t        buffer_line_for_index(Buffer *buffer, int index);
extern void          buffer_build_indices(Buffer *buffer);
extern void          buffer_insert(Buffer *buffer, StringView text, int pos);
extern void          buffer_delete(Buffer *buffer, size_t at, size_t count);
extern void          buffer_merge_lines(Buffer *buffer, int top_line);
extern void          buffer_save(Buffer *buffer);
extern size_t        buffer_word_boundary_left(Buffer *buffer, size_t index);
extern size_t        buffer_word_boundary_right(Buffer *buffer, size_t index);

#endif /* __APP_BUFFER_H__ */
