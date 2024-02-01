/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <allocate.h>
#include <buffer.h>

DECLARE_SHARED_ALLOCATOR(eddy);

DA_IMPL(Index);
DA_IMPL(Buffer);

void buffer_build_indices(Buffer *buffer)
{
    if (!buffer->rebuild_needed) {
        return;
    }
    buffer->lines.size = 0;
    if (buffer->text.view.length == 0) {
        return;
    }
    bool  new_line = false;
    Index index = { 0 };
    index.line = buffer->text.view;
    for (size_t ix = 0; ix < buffer->text.view.length; ++ix) {
        if (new_line) {
            index.line.length = ix - index.index_of;
            da_append_Index(&buffer->lines, index);
            index.index_of = ix;
            index.line.ptr = buffer->text.view.ptr + ix;
            index.line.length = buffer->text.view.length - ix;
        }
        new_line = buffer->text.view.ptr[ix] == '\n';
    }
    da_append_Index(&buffer->lines, index);
    buffer->rebuild_needed = false;
}

size_t buffer_line_for_index(Buffer *buffer, int index)
{
    assert(buffer != NULL);
    Indices *indices = &buffer->lines;
    if (indices->size == 0) {
        return 0;
    }
    size_t line_min = 0;
    size_t line_max = indices->size - 1;
    while (true) {
        size_t line = line_min + (line_max - line_min) / 2;
        if ((line < indices->size - 1 && indices->elements[line].index_of <= index && index < indices->elements[line + 1].index_of) || (line == indices->size - 1 && indices->elements[line].index_of <= index)) {
            return line;
        }
        if (indices->elements[line].index_of > index) {
            line_max = line;
        } else {
            line_min = line + 1;
        }
    }
}

void buffer_insert(Buffer *buffer, StringView text, int pos)
{
    if (pos < 0) {
        pos = 0;
    }
    if (pos > buffer->text.view.length) {
        pos = buffer->text.view.length;
    }
    if (pos < buffer->text.view.length) {
        sb_insert_sv(&buffer->text, text, pos);
    } else {
        sb_append_sv(&buffer->text, text);
    }
    buffer->rebuild_needed = true;
}

void buffer_delete(Buffer *buffer, size_t at, size_t count)
{
    sb_remove(&buffer->text, at, count);
    buffer->rebuild_needed = true;
}

void buffer_merge_lines(Buffer *buffer, int top_line)
{
    if (top_line > buffer->lines.size - 1) {
        return;
    }
    if (top_line < 0) {
        top_line = 0;
    }
    Index line = buffer->lines.elements[top_line];
    ((char *) buffer->text.view.ptr)[line.index_of + line.line.length - 1] = ' ';
    buffer->rebuild_needed = true;
}
