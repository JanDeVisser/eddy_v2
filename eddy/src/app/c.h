/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __APP_C_H__
#define __APP_C_H__

#include <app/mode.h>
#include <lsp/schema/CompletionItem.h>

typedef struct {
    _M;
    OptionalCompletionItems completions;
} CMode;

MODE_CLASS(CMode, c_mode);

#endif /* __APP_C_H__ */
