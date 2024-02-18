/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __LSP_LSP_H__
#define __LSP_LSP_H__

#include <sv.h>
#include <widget.h>

extern void lsp_on_open(int buffer_num);
extern void lsp_did_save(int buffer_num);
extern void lsp_did_close(int buffer_num);
extern void lsp_did_change(int buffer_num, IntVector2 start, IntVector2 end, StringView text);
extern void lsp_semantic_tokens(int buffer_num);
extern int  lsp_format(int buffer_num);

#endif /* __LSP_LSP_H__ */
