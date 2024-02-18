/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <app/c.h>
#include <base/process.h>
#include <buffer.h>
#include <eddy.h>
#include <editor.h>
#include <lsp/lsp.h>
#include <widget.h>
#include <xml.h>

// -- C Mode ----------------------------------------------------------------

SIMPLE_WIDGET_CLASS_DEF(CMode, c_mode);

void c_mode_cmd_format_source(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target->parent->parent;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    eddy_set_message(&eddy, "%.*", SV_ARG(ctx->called_as));

    if (sv_empty(buffer->name)) {
        eddy_set_message(&eddy, "Cannot format untitled buffer");
        return;
    }
    int num = lsp_format(view->buffer_num);
    if (num >= 0) {
        eddy_set_message(&eddy, "Formatted. Made %d changes", num);
    }
}

void c_mode_on_draw(CMode *mode)
{
}

void c_mode_buffer_event_listener(Buffer *buffer, BufferEvent event)
{
    switch (event.type) {
    case ETInsert:
        lsp_did_change(buffer->buffer_ix, event.range.start, event.range.end, buffer_sv_from_ref(buffer, event.insert.text));
        break;
    case ETDelete:
        lsp_did_change(buffer->buffer_ix, event.range.start, event.range.end, sv_null());
        break;
    case ETReplace:
        lsp_did_change(buffer->buffer_ix, event.range.start, event.range.end, buffer_sv_from_ref(buffer, event.replace.replacement));
        break;
    case ETIndexed:
        lsp_semantic_tokens(buffer->buffer_ix);
        break;
    case ETSave:
        lsp_did_save(buffer->buffer_ix);
        break;
    case ETClose:
        lsp_did_close(buffer->buffer_ix);
        break;
    default:
        break;
    }
}

void c_mode_init(CMode *mode)
{
    widget_add_command(mode, sv_from("c-format-source"), c_mode_cmd_format_source,
        (KeyCombo) { KEY_L, KMOD_SHIFT | KMOD_CONTROL });
    mode->handlers.on_draw = (WidgetOnDraw) c_mode_on_draw;
    BufferView *view = (BufferView *) mode->parent;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    buffer_add_listener(buffer, c_mode_buffer_event_listener);
    lsp_on_open(view->buffer_num);
    lsp_semantic_tokens(view->buffer_num);
}

typedef enum {
    CDirectiveStateInit = 0,
    CDirectiveStateIncludeQuote,
    CDirectiveStateMacroName,
} CDirectiveState;

int handle_include_directive(Lexer *lexer)
{
    char const *buffer = lexer_source(lexer).ptr;
    size_t      ix = 0;
    size_t      state = (size_t) lexer->language_data;
    switch (state) {
    case CDirectiveStateInit: {
        while (buffer[ix] == ' ' || buffer[ix] == '\t') {
            ++ix;
        }
        lexer->language_data = (void *) CDirectiveStateIncludeQuote;
        if (ix > 0) {
            lexer_set_current(lexer, (Token) { .kind = TK_WHITESPACE, TC_WHITESPACE, { buffer, ix } });
            return CDirectiveInclude;
        }
        lexer->language_data = (void *) CDirectiveStateIncludeQuote;
    } // Fall through
    case CDirectiveStateIncludeQuote: {
        if (buffer[0] != '<' && buffer[0] != '"') {
            lexer->language_data = NULL;
            return NO_DIRECTIVE;
        }
        char end = (buffer[0] == '<') ? '>' : '"';
        while (buffer[ix] && buffer[ix] != end && buffer[ix] != '\n') {
            ++ix;
        }
        if (buffer[ix] == end) {
            lexer_set_current(lexer, (Token) { .kind = TK_DIRECTIVE_ARG, end, { buffer, ix + 1 } });
        } else {
            lexer_set_current(lexer, (Token) { .kind = TK_DIRECTIVE_ARG, end, { buffer, ix } });
        }
    } break;
    default:
        UNREACHABLE();
    }
    lexer->language_data = NULL;
    return NO_DIRECTIVE;
}

int handle_macro_name_directive(Lexer *lexer)
{
    char const  *buffer = lexer_source(lexer).ptr;
    size_t const state = (size_t) lexer->language_data;
    switch (state) {
    case CDirectiveStateInit: {
        size_t ix = 0;
        while (buffer[ix] == ' ' || buffer[ix] == '\t') {
            ++ix;
        }
        lexer->language_data = (void *) CDirectiveStateMacroName;
        if (ix > 0) {
            lexer_set_current(lexer, (Token) { .kind = TK_WHITESPACE, TC_WHITESPACE, { buffer, ix } });
            return lexer->current_directive - 1;
        }
    } // Fall through
    case CDirectiveStateMacroName: {
        Token t = lexer_peek_next(lexer);
        if (t.kind == TK_IDENTIFIER) {
            t.kind = TK_DIRECTIVE_ARG;
        }
        lexer_set_current(lexer, t);
    } break;
    default:
        UNREACHABLE();
    }
    lexer->language_data = (void *) NULL;
    return NO_DIRECTIVE;
}

int handle_c_directive(Lexer *lexer, int directive)
{
    switch (directive) {
    case CDirectiveInclude:
        return handle_include_directive(lexer);
    case CDirectiveDefine:
    case CDirectiveIfdef:
    case CDirectiveIfndef:
    case CDirectiveElifdef:
    case CDirectiveElifndef:
        return handle_macro_name_directive(lexer);
    default:
        lexer->language_data = (void *) NULL;
        return NO_DIRECTIVE;
    }
}
