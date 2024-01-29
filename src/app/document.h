/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __DOCUMENT_H__
#define __DOCUMENT_H__

#include <sv.h>
#include <command.h>

#define TRANSPOSEDIRECTIONS(S) \
    S(FORWARD) \
    S(REVERSE)

typedef enum {
#undef TRANSPOSEDIRECTION
#define TRANSPOSEDIRECTION(D) TD_##D,
    TRANSPOSEDIRECTIONS(TRANSPOSEDIRECTION)
#undef TRANSPOSEDIRECTION
} TransposeDirection;

typedef struct {
    StringList    extensions;
    StringView    mime_type;
    struct mode *(*mode_builder)(struct document *);
} FileType;

typedef struct document {
    Commands *commands;
    StringView path;
    bool dirty;
    FileType filetype;
    struct mode *mode;

    bool changed;
    Text *text;

    size_t m_screen_top {0};
    size_t m_screen_left {0};
    size_t m_point {0};
    size_t m_mark {0};
    std::string m_find_term;
    bool m_found { true };
    std::vector<EditAction> m_edits;
    std::optional<size_t> m_undo_pointer {};
    std::chrono::milliseconds m_last_parse_time { 0 };

} Document;

#endif /* __DOCUMENT_H__ */
