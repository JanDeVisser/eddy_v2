/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __APP_BUFFER_H__
#define __APP_BUFFER_H__

#include <palette.h>
#include <sv.h>

typedef enum {
    ETCursorMove,
    ETInsert,
    ETDelete,
    ETReplace,
} EditType;

typedef struct {
    size_t index;
    size_t length;
} StringRef;

typedef struct {
    EditType type;
    int      position;
    struct {
        StringRef text;
    } insert;
    struct {
        size_t    count;
        StringRef deleted;
    } delete;
    struct {
        StringRef overwritten;
        StringRef replacement;
    } replace;
} Edit;

DA_WITH_NAME(Edit, Edits);

typedef struct {
    size_t       index;
    size_t       length;
    size_t       line;
    PaletteIndex color;
} DisplayToken;

DA_WITH_NAME(DisplayToken, DisplayTokens);

typedef struct {
    size_t     index_of;
    StringView line;
    size_t     first_token;
    size_t     num_tokens;
} Index;

DA_WITH_NAME(Index, Indices);

typedef struct {
    StringView    name;
    StringBuilder text;
    int           buffer_ix;
    StringBuilder undo_buffer;
    Edits         undo_stack;
    Indices       lines;
    DisplayTokens tokens;
    size_t        saved_version;
    size_t        indexed_version;
    size_t        undo_pointer;
} Buffer;

DA_WITH_NAME(Buffer, Buffers);
ERROR_OR_ALIAS(Buffer, Buffer *);

extern ErrorOrBuffer buffer_open(Buffer *buffer, StringView name);
extern Buffer       *buffer_new(Buffer *buffer);
extern void          buffer_close(Buffer *buffer);
extern size_t        buffer_line_for_index(Buffer *buffer, int index);
extern void          buffer_build_indices(Buffer *buffer);
extern void          buffer_insert(Buffer *buffer, StringView text, int pos);
extern void          buffer_delete(Buffer *buffer, size_t at, size_t count);
extern void          buffer_replace(Buffer *buffer, size_t at, size_t num, StringView replacement);
extern void          buffer_merge_lines(Buffer *buffer, int top_line);
extern void          buffer_save(Buffer *buffer);
extern size_t        buffer_word_boundary_left(Buffer *buffer, size_t index);
extern size_t        buffer_word_boundary_right(Buffer *buffer, size_t index);
extern void          buffer_edit(Buffer *buffer, Edit edit);
extern void          buffer_undo(Buffer *buffer);
extern void          buffer_redo(Buffer *buffer);

#endif /* __APP_BUFFER_H__ */
