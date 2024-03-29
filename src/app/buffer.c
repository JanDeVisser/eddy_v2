/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <ctype.h>

#include <buffer.h>
#include <c.h>
#include <eddy.h>
#include <io.h>
#include <lexer.h>

DA_IMPL(Index);
DA_IMPL(DisplayToken);
DA_IMPL(Buffer);

DA_IMPL(BufferEvent);

void buffer_apply(Buffer *buffer, BufferEvent event);

ErrorOrBuffer buffer_open(Buffer *buffer, StringView name)
{
    buffer->name = sv_copy(name);
    buffer->text.view = TRY_TO(StringView, Buffer, read_file_by_name(name));
    buffer->lines.size = 0;
    buffer_build_indices(buffer);
    RETURN(Buffer, buffer);
}

void buffer_close(Buffer *buffer)
{
    BufferEvent event = { 0 };
    event.type = ETClose;
    buffer_apply(buffer, event);
}

PaletteIndex token_palette_index(Token t)
{
    switch (t.kind) {
    case TK_KEYWORD:
        return PI_KEYWORD;
    case TK_IDENTIFIER:
        return PI_IDENTIFIER;
    case TK_NUMBER:
        return PI_NUMBER;
    case TK_QUOTED_STRING: {
        switch (t.code) {
        case TC_SINGLE_QUOTED_STRING:
        case TC_UNTERMINATED_SINGLE_QUOTED_STRING:
            return PI_CHAR_LITERAL;
        case TC_DOUBLE_QUOTED_STRING:
        case TC_UNTERMINATED_DOUBLE_QUOTED_STRING:
            return PI_STRING;
        default:
            return PI_DEFAULT;
        }
    }
    case TK_SYMBOL:
        return PI_PUNCTUATION;
    case TK_COMMENT:
        return PI_COMMENT;
    case TK_DIRECTIVE:
        return PI_PREPROCESSOR;
    case TK_DIRECTIVE_ARG:
        return PI_PREPROCESSOR_ARG;
    default:
        return PI_DEFAULT;
    }
}

void buffer_build_indices(Buffer *buffer)
{
    assert(buffer->indexed_version <= buffer->version);
    if (buffer->indexed_version == buffer->version && buffer->lines.size > 0) {
        return;
    }
    buffer->lines.size = 0;
    buffer->tokens.size = 0;
    Lexer lexer = lexer_for_language(&c_language);
    lexer.whitespace_significant = true;
    lexer.include_comments = true;
    lexer_push_source(&lexer, buffer->text.view, buffer->name);
    Index *current = da_append_Index(&buffer->lines, (Index) { 0, buffer->text.view });
    size_t lineno = 0;
    // printf("Buffer size: %zu\n", buffer->text.view.length);
    // printf("%5zu: ", lineno);
    while (true) {
        Token t = lexer_next(&lexer);
        lexer_lex(&lexer);
        if (token_matches(t, TK_WHITESPACE, TC_NEWLINE) || token_matches_kind(t, TK_END_OF_FILE)) {
            // if (current->num_tokens == 0) {
            //     printf("[EOL]\n");
            // } else {
            //     printf("[EOL] %zu..%zu\n", current->first_token, current->first_token + current->num_tokens-1);
            // }
            current->line.length = t.location - current->index_of;
            if (t.kind == TK_END_OF_FILE) {
                break;
            }
            current = da_append_Index(&buffer->lines, (Index) { t.location + 1, { buffer->text.view.ptr + t.location + 1, 0 }, 0, 0 });
            // ++lineno;
            // printf("%5zu: ", lineno);
            continue;
        }
        PaletteIndex pi = token_palette_index(t);
        // if (token_matches(t, TK_WHITESPACE, TC_WHITESPACE)) {
        //     printf("%*.s", (int) t.text.length, "");
        // } else {
        //     printf("[%.*s %s %s]", SV_ARG(t.text), TokenKind_name(t.kind), PaletteIndex_name(pi));
        // }
        if (current->num_tokens == 0) {
            current->first_token = buffer->tokens.size;
        }
        ++current->num_tokens;
        da_append_DisplayToken(&buffer->tokens, (DisplayToken) { t.location, t.text.length, lineno, pi });
    }
    // printf("\n[EOF]\n=====================\n");
    buffer->indexed_version = buffer->version;
    BufferEvent event = { .type = ETIndexed };
    for (BufferEventListenerList *list_entry = buffer->listeners; list_entry != NULL; list_entry = list_entry->next) {
        list_entry->listener(buffer, event);
    }
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

IntVector2 buffer_index_to_position(Buffer *buffer, int index)
{
    IntVector2 ret = { 0 };
    ret.line = buffer_line_for_index(buffer, index);
    ret.column = index - buffer->lines.elements[ret.line].index_of;
    return ret;
}

size_t buffer_position_to_index(Buffer *buffer, IntVector2 position)
{
    Index *line = buffer->lines.elements + position.line;
    return line->index_of + position.column;
}

StringRef append_to_undo_buffer(Buffer *buffer, StringView sv)
{
    if (sv.ptr < buffer->undo_buffer.view.ptr || sv.ptr > buffer->undo_buffer.view.ptr + buffer->undo_buffer.view.length) {
        size_t buffer_pos = buffer->undo_buffer.view.length;
        sb_append_sv(&buffer->undo_buffer, sv);
        return (StringRef) { buffer_pos, sv.length };
    }
    return (StringRef) { sv.ptr - buffer->undo_buffer.view.ptr, sv.length };
}

StringRef append_to_undo_buffer_from_text(Buffer *buffer, size_t at, size_t count)
{
    return append_to_undo_buffer(buffer, (StringView) { buffer->text.view.ptr + at, count });
}

StringView buffer_sv_from_ref(Buffer *buffer, StringRef ref)
{
    if (ref.length == 0) {
        return sv_null();
    }
    return (StringView) { buffer->undo_buffer.view.ptr + ref.index, ref.length };
}

void buffer_apply(Buffer *buffer, BufferEvent event)
{
    switch (event.type) {
    case ETInsert: {
        if (event.insert.text.length == 0) {
            return;
        }
        event.range.start = buffer_index_to_position(buffer, event.position);
        event.range.end = event.range.start;
        StringView sv = buffer_sv_from_ref(buffer, event.insert.text);
        if (event.position < buffer->text.view.length) {
            sb_insert_sv(&buffer->text, sv, event.position);
        } else {
            sb_append_sv(&buffer->text, sv);
        }
        ++buffer->version;
    } break;
    case ETDelete: {
        if (event.delete.count == 0) {
            return;
        }
        event.range.start = buffer_index_to_position(buffer, event.position);
        event.range.end = buffer_index_to_position(buffer, event.position + event.delete.count);
        sb_remove(&buffer->text, event.position, event.delete.count);
        ++buffer->version;
    } break;
    case ETReplace: {
        if (event.replace.replacement.length == 0) {
            return;
        }
        event.range.start = buffer_index_to_position(buffer, event.position);
        event.range.end = buffer_index_to_position(buffer, event.position + event.replace.overwritten.length);
        sb_remove(&buffer->text, event.position, event.replace.overwritten.length);
        StringView sv = buffer_sv_from_ref(buffer, event.replace.replacement);
        if (event.position < buffer->text.view.length) {
            sb_insert_sv(&buffer->text, sv, event.position);
        } else {
            sb_append_sv(&buffer->text, sv);
        }
        ++buffer->version;
    } break;
    case ETSave: {
        assert(buffer->saved_version <= buffer->version);
        StringView name = buffer_sv_from_ref(buffer, event.save.file_name);
        if (sv_empty(name) && buffer->saved_version == buffer->version) {
            return;
        }
        if (sv_not_empty(name)) {
            buffer->name = sv_copy(name);
            sv_free(buffer->uri);
        }
        if (sv_empty(buffer->name)) {
            return;
        }
        MUST(Size, write_file_by_name(buffer->name, buffer->text.view));
        buffer->saved_version = buffer->undo_stack.size;
    } break;
    case ETClose: {
        for (BufferEventListenerList *list_entry = buffer->listeners; list_entry != NULL; list_entry = list_entry->next) {
            list_entry->listener(buffer, event);
        }
        sv_free(buffer->text.view);
        sv_free(buffer->undo_buffer.view);
        sv_free(buffer->name);
        sv_free(buffer->uri);
        da_free_BufferEvent(&buffer->undo_stack);
        da_free_DisplayToken(&buffer->tokens);
        da_free_Index(&buffer->lines);
        for (BufferEventListenerList *entry = buffer->listeners; entry;) {
            BufferEventListenerList *next = entry->next;
            free(entry);
            entry = next;
        }
        memset(buffer, 0, sizeof(Buffer));
        return;
    } break;
    default:
        break;
    }
    for (BufferEventListenerList *list_entry = buffer->listeners; list_entry != NULL; list_entry = list_entry->next) {
        list_entry->listener(buffer, event);
    }
}

void buffer_edit(Buffer *buffer, BufferEvent event)
{
    switch (event.type) {
    case ETInsert: {
        if (event.insert.text.length == 0) {
            return;
        }
        event.position = iclamp(event.position, 0, buffer->text.view.length);
    } break;
    case ETDelete: {
        event.position = iclamp(event.position, 0, buffer->text.view.length);
        event.delete.count = iclamp(event.delete.count, 0, buffer->text.view.length - event.position);
        if (event.delete.count == 0) {
            return;
        }
        event.delete.deleted = append_to_undo_buffer_from_text(buffer, event.position, event.delete.count);
    } break;
    case ETReplace: {
        event.position = iclamp(event.position, 0, buffer->text.view.length);
        int count = iclamp(event.replace.overwritten.length, 0, buffer->text.view.length - event.position);
        if (count <= 0) {
            return;
        }
        event.replace.overwritten = append_to_undo_buffer_from_text(buffer, event.position, count);
    } break;
    default:
        break;
    }
    buffer_apply(buffer, event);
    da_append_BufferEvent(&buffer->undo_stack, event);
    buffer->undo_pointer = buffer->undo_stack.size;
}

BufferEvent revert_edit(BufferEvent event)
{
    BufferEvent ret = { 0 };
    switch (event.type) {
    case ETInsert:
        ret.type = ETDelete;
        ret.position = event.position;
        ret.delete.count = event.insert.text.length;
        break;
    case ETDelete:
        ret.type = ETInsert;
        ret.position = event.position;
        ret.insert.text = event.delete.deleted;
        break;
    case ETReplace:
        ret.type = ETReplace;
        ret.position = event.position;
        ret.replace.overwritten = event.replace.replacement;
        ret.replace.replacement = event.replace.overwritten;
        break;
    default:
        break;
    }
    return ret;
}

void buffer_undo(Buffer *buffer)
{
    if (!buffer->undo_pointer) {
        return;
    }
    buffer_apply(buffer, revert_edit(buffer->undo_stack.elements[--buffer->undo_pointer]));
}

void buffer_redo(Buffer *buffer)
{
    if (buffer->undo_pointer >= buffer->undo_stack.size) {
        return;
    }
    buffer_apply(buffer, revert_edit(buffer->undo_stack.elements[buffer->undo_pointer++]));
}

void buffer_insert(Buffer *buffer, StringView text, int pos)
{
    BufferEvent event = { 0 };
    event.type = ETInsert;
    event.position = pos;
    event.insert.text = append_to_undo_buffer(buffer, text);
    buffer_edit(buffer, event);
}

void buffer_delete(Buffer *buffer, size_t at, size_t count)
{
    BufferEvent event = { 0 };
    event.type = ETDelete;
    event.position = at;
    event.delete.count = count;
    buffer_edit(buffer, event);
}

void buffer_replace(Buffer *buffer, size_t at, size_t num, StringView replacement)
{
    BufferEvent event = { 0 };
    event.type = ETReplace;
    event.position = at;
    event.replace.replacement = append_to_undo_buffer(buffer, replacement);
    event.replace.overwritten.length = num;
    buffer_edit(buffer, event);
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
    buffer_replace(buffer, line.index_of + line.line.length, 1, sv_from(" "));
}

void buffer_save(Buffer *buffer)
{
    BufferEvent event = { 0 };
    event.type = ETSave;
    event.position = 0;
    buffer_apply(buffer, event);
}

void buffer_save_as(Buffer *buffer, StringView name)
{
    BufferEvent event = { 0 };
    event.type = ETSave;
    event.position = 0;
    event.save.file_name = append_to_undo_buffer(buffer, name);
    buffer_apply(buffer, event);
}

size_t buffer_word_boundary_left(Buffer *buffer, size_t index)
{
    if (isalnum(buffer->text.view.ptr[index]) || buffer->text.view.ptr[index] == '_') {
        while (((int) index) > 0 && (isalnum(buffer->text.view.ptr[index]) || buffer->text.view.ptr[index] == '_')) {
            --index;
        }
        ++index;
    } else {
        while (((int) index) > 0 && (!isalnum(buffer->text.view.ptr[index]) && buffer->text.view.ptr[index] != '_')) {
            --index;
        }
        ++index;
    }
    return index;
}

size_t buffer_word_boundary_right(Buffer *buffer, size_t index)
{
    size_t max_index = buffer->text.view.length;
    if (isalnum(buffer->text.view.ptr[index]) || buffer->text.view.ptr[index] == '_') {
        while (index < max_index && (isalnum(buffer->text.view.ptr[index]) || buffer->text.view.ptr[index] == '_')) {
            ++index;
        }
    } else {
        while (index < max_index && (!isalnum(buffer->text.view.ptr[index]) && buffer->text.view.ptr[index] != '_')) {
            ++index;
        }
    }
    return index;
}

void buffer_add_listener(Buffer *buffer, BufferEventListener listener)
{
    BufferEventListenerList *list_entry = MALLOC(BufferEventListenerList);
    list_entry->listener = listener;
    list_entry->next = buffer->listeners;
    buffer->listeners = list_entry;
}

StringView buffer_uri(Buffer *buffer)
{
    if (sv_empty(buffer->uri) && sv_not_empty(buffer->name)) {
        buffer->uri = sv_printf("file://%.*s/%.*s", SV_ARG(eddy.project_dir), SV_ARG(buffer->name));
    }
    return buffer->uri;
}
