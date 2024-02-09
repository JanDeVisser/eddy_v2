/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <allocate.h>
#include <buffer.h>
#include <ctype.h>
#include <io.h>

DECLARE_SHARED_ALLOCATOR(eddy);

DA_IMPL(Index);
DA_IMPL(Buffer);

ErrorOrBuffer buffer_open(Buffer *buffer, StringView name)
{
    buffer->name = name;
    buffer->text.view = TRY_TO(StringView, Buffer, read_file_by_name(name));
    buffer->rebuild_needed = true;
    RETURN(Buffer, buffer);
}

Buffer * buffer_new(Buffer *buffer)
{
    memset(buffer , 0, sizeof(Buffer));
    buffer->text = sb_create();
    buffer->rebuild_needed = true;
    return buffer;
}

void buffer_build_indices(Buffer *buffer)
{
    if (!buffer->rebuild_needed) {
        return;
    }
    buffer->lines.size = 0;
    da_append_Index(&buffer->lines, (Index) { 0, buffer->text.view });
    size_t lineno = 0;
    for (size_t ix = 0; ix < buffer->text.view.length; ++ix) {
        if (buffer->text.view.ptr[ix] == '\n') {
            buffer->lines.elements[lineno].line.length = ix - buffer->lines.elements[lineno].index_of + 1;
            da_append_Index(&buffer->lines, (Index) { ix + 1, { buffer->text.view.ptr + ix + 1, 0 } });
            ++lineno;
        }
    }
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
    buffer->dirty = true;
    buffer->rebuild_needed = true;
}

void buffer_delete(Buffer *buffer, size_t at, size_t count)
{
    sb_remove(&buffer->text, at, count);
    buffer->dirty = true;
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
    buffer->dirty = true;
    buffer->rebuild_needed = true;
}

void buffer_save(Buffer *buffer)
{
    if (!buffer->dirty) {
        return;
    }
    if (sv_empty(buffer->name)) {
        return;
    }
    write_file_by_name(buffer->name, buffer->text.view);
    buffer->dirty = false;
}

size_t buffer_word_boundary_left(Buffer *buffer, size_t index)
{
    if (isalnum(buffer->text.view.ptr[index]) || buffer->text.view.ptr[index] == '_') {
        while (index > 0 && (isalnum(buffer->text.view.ptr[index - 1]) || buffer->text.view.ptr[index - 1] == '_')) {
            --index;
        }
    } else {
        while (index > 0 && (!isalnum(buffer->text.view.ptr[index - 1]) && buffer->text.view.ptr[index - 1] != '_')) {
            --index;
        }
    }
    return index;
}

size_t buffer_word_boundary_right(Buffer *buffer, size_t index)
{
    size_t max_index = buffer->text.view.length;
    if (isalnum(buffer->text.view.ptr[index]) || buffer->text.view.ptr[index] == '_') {
        while (index < max_index && (isalnum(buffer->text.view.ptr[index + 1]) || buffer->text.view.ptr[index + 1] == '_')) {
            ++index;
        }
    } else {
        while (index < max_index && (!isalnum(buffer->text.view.ptr[index + 1]) && buffer->text.view.ptr[index + 1] != '_')) {
            ++index;
        }
    }
    return index;
}
