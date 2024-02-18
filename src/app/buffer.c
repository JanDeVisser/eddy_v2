/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <buffer.h>
#include <c.h>
#include <ctype.h>
#include <io.h>
#include <lexer.h>

DA_IMPL(Index);
DA_IMPL(DisplayToken);
DA_IMPL(Buffer);

DA_IMPL(Edit);

ErrorOrBuffer buffer_open(Buffer *buffer, StringView name)
{
    buffer->name = name;
    buffer->text.view = TRY_TO(StringView, Buffer, read_file_by_name(name));
    buffer->lines.size = 0;
    buffer_build_indices(buffer);
    RETURN(Buffer, buffer);
}

void buffer_close(Buffer *buffer)
{
    sv_free(buffer->text.view);
    sv_free(buffer->undo_buffer.view);
    free(buffer->undo_stack.elements);
    free(buffer->tokens.elements);
    free(buffer->lines.elements);
    memset(buffer, 0, sizeof(Buffer));
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
    assert(buffer->indexed_version <= buffer->undo_stack.size);
    if (buffer->indexed_version == buffer->undo_stack.size && buffer->lines.size > 0) {
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
    for (Token t = lexer_next(&lexer); t.kind != TK_END_OF_FILE; t = lexer_next(&lexer)) {
        lexer_lex(&lexer);
        if (token_matches(t, TK_WHITESPACE, TC_NEWLINE)) {
            // if (current->num_tokens == 0) {
            //     printf("[EOL]\n");
            // } else {
            //     printf("[EOL] %zu..%zu\n", current->first_token, current->first_token + current->num_tokens-1);
            // }
            buffer->lines.elements[lineno].line.length = t.location - buffer->lines.elements[lineno].index_of + 1;
            current = da_append_Index(&buffer->lines, (Index) { t.location + 1, { buffer->text.view.ptr + t.location + 1, 0 }, 0, 0 });
            // ++lineno;
            printf("%5zu: ", lineno);
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
    buffer->indexed_version = buffer->undo_stack.size;
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

StringView sv_from_ref(Buffer *buffer, StringRef ref)
{
    return (StringView) { buffer->undo_buffer.view.ptr + ref.index, ref.length };
}

void buffer_apply(Buffer *buffer, Edit edit)
{
    switch (edit.type) {
    case ETInsert: {
        if (edit.insert.text.length == 0) {
            return;
        }
        StringView sv = sv_from_ref(buffer, edit.insert.text);
        if (edit.position < buffer->text.view.length) {
            sb_insert_sv(&buffer->text, sv, edit.position);
        } else {
            sb_append_sv(&buffer->text, sv);
        }
    } break;
    case ETDelete: {
        if (edit.delete.count == 0) {
            return;
        }
        sb_remove(&buffer->text, edit.position, edit.delete.count);
    } break;
    case ETReplace: {
        if (edit.replace.replacement.length == 0) {
            return;
        }
        sb_remove(&buffer->text, edit.position, edit.replace.overwritten.length);
        StringView sv = sv_from_ref(buffer, edit.replace.replacement);
        if (edit.position < buffer->text.view.length) {
            sb_insert_sv(&buffer->text, sv, edit.position);
        } else {
            sb_append_sv(&buffer->text, sv);
        }
    } break;
    default:
        break;
    }
}

void buffer_edit(Buffer *buffer, Edit edit)
{
    switch (edit.type) {
    case ETInsert: {
        if (edit.insert.text.length == 0) {
            return;
        }
        edit.position = iclamp(edit.position, 0, buffer->text.view.length);
    } break;
    case ETDelete: {
        edit.position = iclamp(edit.position, 0, buffer->text.view.length);
        edit.delete.count = iclamp(edit.delete.count, 0, buffer->text.view.length - edit.position);
        if (edit.delete.count == 0) {
            return;
        }
        edit.delete.deleted = append_to_undo_buffer_from_text(buffer, edit.position, edit.delete.count);
    } break;
    case ETReplace: {
        edit.position = iclamp(edit.position, 0, buffer->text.view.length);
        int count = iclamp(edit.replace.overwritten.length, 0, buffer->text.view.length - edit.position);
        if (count <= 0) {
            return;
        }
        edit.replace.overwritten = append_to_undo_buffer_from_text(buffer, edit.position, count);
    } break;
    default:
        break;
    }
    buffer_apply(buffer, edit);
    da_append_Edit(&buffer->undo_stack, edit);
    buffer->undo_pointer = buffer->undo_stack.size;
}

Edit revert_edit(Edit edit)
{
    Edit ret = {0};
    switch (edit.type) {
    case ETInsert:
        ret.type = ETDelete;
        ret.position = edit.position;
        ret.delete.count = edit.insert.text.length;
        break;
    case ETDelete:
        ret.type = ETInsert;
        ret.position = edit.position;
        ret.insert.text = edit.delete.deleted;
        break;
    case ETReplace:
        ret.type = ETReplace;
        ret.position = edit.position;
        ret.replace.overwritten = edit.replace.replacement;
        ret.replace.replacement = edit.replace.overwritten;
        break;
    default:
        UNREACHABLE();
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
    Edit edit = {0};
    edit.type = ETInsert;
    edit.position = pos;
    edit.insert.text = append_to_undo_buffer(buffer, text);
    buffer_edit(buffer, edit);
}

void buffer_delete(Buffer *buffer, size_t at, size_t count)
{
    Edit edit = {0};
    edit.type = ETDelete;
    edit.position = at;
    edit.delete.count = count;
    buffer_edit(buffer, edit);
}

void buffer_replace(Buffer *buffer, size_t at, size_t num, StringView replacement)
{
    Edit edit = {0};
    edit.type = ETReplace;
    edit.position = at;
    edit.replace.replacement = append_to_undo_buffer(buffer, replacement);
    edit.replace.overwritten.length = num;
    buffer_edit(buffer, edit);
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
    assert(buffer->saved_version <= buffer->undo_stack.size);
    if (buffer->saved_version == buffer->undo_stack.size) {
        return;
    }
    if (sv_empty(buffer->name)) {
        return;
    }
    write_file_by_name(buffer->name, buffer->text.view);
    buffer->saved_version = buffer->undo_stack.size;
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
