/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <ctype.h>

#define STATIC_ALLOCATOR
#include <allocate.h>
#include <text.h>

DA_IMPL(LineToken);
DA_IMPL(Line);

void document_position_clear(DocumentPosition *position)
{
    position->line = position->column = 0;
}

size_t line_token_end_offset(LineToken *token)
{
    return token->offset + token->length;
}

size_t line_offset(Line *line)
{
    return (line->size == 0) ? 0 : line->elements[0].offset;
}

size_t line_end_offset(Line *line)
{
    return (line->size == 0) ? 0 : line_token_end_offset(line->elements + (line->size - 1));
}

size_t line_length(Line *line)
{
    return line_end_offset(line) - line_offset(line);
}

StringView line_text(Line *line)
{
    return sv_substring(line->text->text.view, line_offset(line), line_length(line));
}

LineTokens line_tokens(Line *line)
{
    LineTokens ret = { 0 };
    for (size_t ix = 0; ix < line->size; ++ix) {
        da_append_LineToken(&ret, line->elements[ix]);
    }
    return ret;
}

size_t text_length(Text *text)
{
    return text->text.view.length;
}

char text_char_text_at(Text *text, size_t ix)
{
    assert(ix < text->text.view.length);
    return text->text.view.ptr[ix];
}

size_t text_line_length(Text *text, size_t line)
{
    assert(line < text->size);
    return line_length(text->elements + line);
}

size_t text_offset_of(Text *text, size_t line)
{
    assert(line < text->size);
    return line_offset(text->elements + line);
}

size_t text_offset_of_end(Text *text, size_t line)
{
    assert(line < text->size);
    return line_end_offset(text->elements + line);
}

size_t text_find_line_number(Text *text, size_t of_offset)
{
    if (sv_empty(text->text.view)) {
        return 0;
    }
    assert(of_offset < text->text.view.length);
    size_t line_min = 0;
    size_t line_max = text->size - 1;
    while (true) {
        size_t line = line_min + (line_max - line_min) / 2;
        size_t offset = line_offset(text->elements + line);
        if ((line < text->size - 1 && offset <= of_offset && of_offset < text_offset_of(text, line + 1)) || (line == text->size() - 1 && offset <= of_offset)) {
            return line;
        }
        if (offset > of_offset) {
            line_max = line;
        } else {
            line_min = line + 1;
        }
    }
}

size_t text_word_boundary_left(Text *text, size_t index)
{
    assert(index < text->text.view.length);
    if (sv_empty(text->text.view)) {
        return 0;
    }
    if (isalnum(text->text.view.ptr[index]) || text->text.view.ptr[index] == '_') {
        while (index > 0 && (isalnum(text->text.view.ptr[index - 1]) || text->text.view.ptr[index - 1] == '_'))
            --index;
    } else {
        while (index > 0 && (!isalnum(text->text.view.ptr[index - 1]) && text->text.view.ptr[index - 1] != '_'))
            --index;
    }
    return index;
}

size_t text_word_boundary_right(Text *text, size_t index)
{
    auto max_index = text->text.view.length - 1;
    if (isalnum(text->text.view.ptr[index]) || text->text.view.ptr[index] == '_') {
        while (index < max_index && (isalnum(text->text.view.ptr[index + 1]) || text->text.view.ptr[index + 1] == '_'))
            ++index;
    } else {
        while (index < max_index && !isalnum(text->text.view.ptr[index + 1] && text->text.view.ptr[index + 1] != '_'))
            ++index;
    }
    return index;
}

StringView text_substring(Text *text, size_t from, size_t length)
{
    return sv_substring(text->text.view, from, length);
}

OptionalUInt64 text_find(Text *text, StringView find_term, size_t offset)
{
    int ret = sv_find(sv_substring(text->text.view, offset, text->text.view.length), find_term);
    if (ret < 0) {
        RETURN_EMPTY(UInt64);
    }
    RETURN_VALUE(UInt64, (uint64_t) ret);
}

void text_insert(Text *text, size_t at, StringView str)
{
    // std::lock_guard const lock { m_lock };
    sb_insert_sv(&text->text, str, at);
    text->dirty = true;
}

void text_insert_ch(Text *text, size_t at, char ch)
{
    // std::lock_guard const lock { m_lock };
    sb_insert_chars(&text->text, &ch, 1, at);
    text->dirty = true;
}

void text_remove(Text *text, size_t at, size_t num)
{
    // std::lock_guard const lock { m_lock };
    sb_remove(&text->text, at, num);
    text->dirty = true;
}

void text_join_lines(Text *text, size_t first_line)
{
    if (first_line >= text->size - 1) {
        return;
    }
    auto ix = line_end_offset(text->elements + first_line);
    assert(text->text.view.ptr[ix] == '\n');
    text_remove(text, ix, 1);
}

void text_duplicate_line(Text *text, size_t line)
{
    if (line >= text->size) {
        return;
    }
    if (line == text->size - 1) {
        text_insert(text, text->text.view.length - 1, sv_from("\n"));
    }
    StringView l = line_text(text->elements + line);
    assert(l.ptr[l.length] == '\n');
    ++l.length;
    text_insert(text, text_offset_of(text, line) + l.length, l);
}

void text_transpose_lines(Text *text, size_t first_line)
{
    if ((text->size < 2) || (first_line > text->size - 2)) {
        return;
    }
    size_t     offset_1 = text_offset_of(text, first_line);
    size_t     offset_2 = text_offset_of(text, first_line + 1);
    size_t     len_1 = line_length(text->elements + first_line);
    size_t     len_2 = line_length(text->elements + (first_line + 1));
    StringView first = sv_copy(sv_substring(text->text.view, offset_1, len_1));
    StringView second = sv_copy(sv_substring(text->text.view, offset_2, len_2));
    text_remove(text, offset_1, len_1 + len_2 + 1);
    text_insert(text, offset_1, second);
    text_insert(text, offset_1 + len_2, sv_from("\n"));
    text_insert(text, offset_1 + len_2 + 1, first);
    sv_free(first);
    sv_free(second);
}

void text_clear(Text *text)
{
    memset(text->elements, 0, text->size * sizeof(Line));
    text->size = 0;
    text->text.view.length = 0;
    text->dirty = true;
}

void text_assign(Text *text, StringView txt)
{
    text_clear(text);
    sb_append_sv(&text->text, txt);
}

void text_parse(Text *text, LexerDef lexer_def)
{
    // std::lock_guard const lock { m_lock };
    if (!text->dirty) {
        return;
    }
    memset(text->elements, 0, text->size * sizeof(Line));
    text->size = 0;
    void *lexer = lexer_def.constructor(text->text.view);
    for (LineToken token = lexer_def.lex(lexer); token.length != 0; token = lexer_def.lex(lexer)) {
        StringView t = { .ptr = text->text.view.ptr + token.offset, .length = token.length };
        if (sv_eq_cstr(t, "\n")) {
            Line l = {0};
            l.text = text;
            DIA_APPEND(Line, text, l);
            break;
        }
        DIA_APPEND(LineToken, (text->elements + text->size), token);
    }
    text->dirty = false;
}
