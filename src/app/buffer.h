/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __APP_BUFFER_H__
#define __APP_BUFFER_H__

#include <palette.h>
#include <sv.h>
#include <widget.h>

typedef enum {
    ETCursorMove,
    ETInsert,
    ETDelete,
    ETReplace,
    ETIndexed,
    ETSave,
    ETClose,
} BufferEventType;

typedef struct {
    IntVector2 start;
    IntVector2 end;
} EventRange;

typedef struct {
    BufferEventType type;
    int             position;
    EventRange      range;
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
    struct {
        StringRef file_name;
    } save;
} BufferEvent;

DA_WITH_NAME(BufferEvent, BufferEvents);

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

typedef struct buffer Buffer;
typedef void (*BufferEventListener)(Buffer *, BufferEvent);

typedef struct _buffer_event_listener_list {
    BufferEventListener                 listener;
    struct _buffer_event_listener_list *next;
} BufferEventListenerList;

typedef struct buffer {
    StringView               name;
    StringView               uri;
    StringBuilder            text;
    int                      buffer_ix;
    StringBuilder            undo_buffer;
    BufferEvents             undo_stack;
    Indices                  lines;
    DisplayTokens            tokens;
    size_t                   saved_version;
    size_t                   indexed_version;
    size_t                   version;
    size_t                   undo_pointer;
    BufferEventListenerList *listeners;
} Buffer;

DA_WITH_NAME(Buffer, Buffers);
ERROR_OR_ALIAS(Buffer, Buffer *);

extern ErrorOrBuffer buffer_open(Buffer *buffer, StringView name);
extern Buffer       *buffer_new(Buffer *buffer);
extern void          buffer_close(Buffer *buffer);
extern size_t        buffer_line_for_index(Buffer *buffer, int index);
extern void          buffer_build_indices(Buffer *buffer);
extern size_t        buffer_position_to_index(Buffer *buffer, IntVector2 position);
extern IntVector2    buffer_index_to_position(Buffer *buffer, int index);
extern void          buffer_insert(Buffer *buffer, StringView text, int pos);
extern void          buffer_delete(Buffer *buffer, size_t at, size_t count);
extern void          buffer_replace(Buffer *buffer, size_t at, size_t num, StringView replacement);
extern void          buffer_merge_lines(Buffer *buffer, int top_line);
extern void          buffer_save(Buffer *buffer);
extern size_t        buffer_word_boundary_left(Buffer *buffer, size_t index);
extern size_t        buffer_word_boundary_right(Buffer *buffer, size_t index);
extern void          buffer_edit(Buffer *buffer, BufferEvent event);
extern void          buffer_undo(Buffer *buffer);
extern void          buffer_redo(Buffer *buffer);
extern StringView    buffer_sv_from_ref(Buffer *buffer, StringRef ref);
extern void          buffer_add_listener(Buffer *buffer, BufferEventListener listener);
extern StringView    buffer_uri(Buffer *buffer);

#endif /* __APP_BUFFER_H__ */
