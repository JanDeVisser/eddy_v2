/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef APP_EVENT_H
#define APP_EVENT_H

#include <app/widget.h>

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


typedef struct buffer Buffer;
typedef void (*BufferEventListener)(Buffer *, BufferEvent);

typedef struct buffer_event_listener_list {
    BufferEventListener                listener;
    struct buffer_event_listener_list *next;
} BufferEventListenerList;

#endif /* APP_EVENT_H */
