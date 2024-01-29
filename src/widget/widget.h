/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __WIDGET_H__
#define __WIDGET_H__

#include <raylib.h>

#define TEXT_ALIGNMENTS(S) \
    S(LEFT)                \
    S(RIGHT)               \
    S(CENTER)

typedef enum {
#undef TEXT_ALIGNMENT
#define TEXT_ALIGNMENT(A) TA_##A,
    TEXT_ALIGNMENTS(TEXT_ALIGNMENT)
#undef TEXT_ALIGNMENT
} text_alignment_t;

typedef struct _widget {
    struct _widget *parent;
    Rectangle rect;
} widget_t;

#endif /* __WIDGET_H__ */
