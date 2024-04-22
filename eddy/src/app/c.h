/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef APP_C_H
#define APP_C_H

#include <app/mode.h>
#include <lsp/lsp.h>
#include <lsp/schema/CompletionItem.h>

typedef Mode CMode;

MODE_CLASS(CMode, c_mode);

typedef struct {
    MODE_DATA;
    OptionalCompletionItems completions;
} CModeData;

MODE_DATA_CLASS(CModeData, c_mode_data);

#endif /* APP_C_H */

