/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <buffer.h>
#include <ctype.h>
#include <io.h>
#include <lang/c.h>
#include <lexer.h>

DA_IMPL(Index);
DA_IMPL(DisplayToken);
DA_IMPL(Buffer);

ErrorOrBuffer buffer_open(Buffer *buffer, StringView name)
{
    buffer->name = name;
    buffer->text.view = TRY_TO(StringView, Buffer, read_file_by_name(name));
    buffer->rebuild_needed = true;
    RETURN(Buffer, buffer);
}

Buffer *buffer_new(Buffer *buffer)
{
    memset(buffer, 0, sizeof(Buffer));
    buffer->text = sb_create();
    buffer->rebuild_needed = true;
    return buffer;
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
    if (!buffer->rebuild_needed) {
        return;
    }
    buffer->lines.size = 0;
    buffer->tokens.size = 0;
    Lexer lexer = lexer_for_language(&c_language);
    lexer.whitespace_significant = true;
    lexer.include_comments = true;
    lexer_push_source(&lexer, buffer->text.view, buffer->name);
    Index *current = da_append_Index(&buffer->lines, (Index) { 0, buffer->text.view });
    ;
    size_t lineno = 0;
    //    printf("%5zu: ", lineno);
    for (Token t = lexer_next(&lexer); t.kind != TK_END_OF_FILE; t = lexer_next(&lexer)) {
        lexer_lex(&lexer);
        if (token_matches(t, TK_WHITESPACE, TC_NEWLINE)) {
            //            if (current->num_tokens == 0) {
            //                printf("[EOL]\n");
            //            } else {
            //                printf("[EOL] %zu..%zu\n", current->first_token, current->first_token + current->num_tokens-1);
            //            }
            buffer->lines.elements[lineno].line.length = t.location - buffer->lines.elements[lineno].index_of + 1;
            current = da_append_Index(&buffer->lines, (Index) { t.location + 1, { buffer->text.view.ptr + t.location + 1, 0 }, 0, 0 });
            ++lineno;
            //            printf("%5zu: ", lineno);
            continue;
        }
        PaletteIndex pi = token_palette_index(t);
        //        if (token_matches(t, TK_WHITESPACE, TC_WHITESPACE)) {
        //            printf("%*.s", (int) t.text.length, "");
        //        } else {
        //            printf("[%.*s %s %s]", SV_ARG(t.text), TokenKind_name(t.kind), PaletteIndex_name(pi));
        //        }
        if (current->num_tokens == 0) {
            current->first_token = buffer->tokens.size;
        }
        ++current->num_tokens;
        da_append_DisplayToken(&buffer->tokens, (DisplayToken) { t.location, t.text.length, lineno, pi });
    }
    //    printf("\n[EOF]\n=====================\n");
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
