/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __TEXT_H__
#define __TEXT_H__

#include <stdlib.h>

#include <palette.h>
#include <sv.h>

typedef struct {
    size_t line;
    size_t column;
} DocumentPosition;

typedef struct {
    size_t       offset;
    size_t       length;
    PaletteIndex color;
} LineToken;

DA_WITH_NAME(LineToken, LineTokens);

typedef struct {
                 DIA(LineToken);
    struct text *text;
    int          start_level;
    int          end_level;
} Line;

DA_WITH_NAME(Line, Lines);

typedef struct text {
                  DIA(Line);
    StringBuilder text;
    bool          dirty;
    size_t        offset;
} Text;

// template<class Mode>
// void Text::append_token(Mode const &mode, Token const &token)
// {
//     size_t       length = token.length();
//     PaletteIndex color = mode.color_for(token.code());
//     back().emplace_back(m_offset, std::string_view(m_text).substr(m_offset, length), color);
//     m_offset += length;
// }

typedef void     *(*CreateLexer)(StringView);
typedef LineToken (*Lex)(void *);

typedef struct {
    CreateLexer constructor;
    Lex         lex;
} LexerDef;

extern void           document_position_clear(DocumentPosition *position);
extern size_t         line_token_end_offset(LineToken *token);
extern size_t         line_offset(Line *line);
extern size_t         line_end_offset(Line *line);
extern size_t         line_length(Line *line);
extern LineTokens     line_tokens(Line *line);
extern StringView     line_text(Line *line);
extern size_t         text_length(Text *text);
extern char           text_char_at(Text *text, size_t ix);
extern size_t         text_line_length(Text *text, size_t line);
extern size_t         text_offset_of(Text *text, size_t line);
extern size_t         text_offset_of_end(Text *text, size_t line);
extern size_t         text_find_line_number(Text *text, size_t offset);
extern size_t         text_word_boundary_left(Text *text, size_t index);
extern size_t         text_word_boundary_right(Text *text, size_t index);
extern StringView     text_substring(Text *text, size_t from, size_t length);
extern OptionalUInt64 text_find(Text *text, StringView find_term, size_t offset);
extern void           text_insert(Text *text, size_t at, StringView str);
extern void           text_insert_ch(Text *text, size_t at, char ch);
extern void           text_remove(Text *text, size_t at, size_t num);
extern void           text_join_lines(Text *text, size_t first_line);
extern void           text_duplicate_line(Text *text, size_t line);
extern void           text_transpose_lines(Text *text, size_t first_line);
extern void           text_clear(Text *text);
extern void           text_assign(Text *text, StringView txt);
extern void           text_parse(Text *text, LexerDef lexer_def);

#endif /* __TEXT_H__ */
