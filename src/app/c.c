/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "lsp/schema/TextEdit.h"
#include "raylib.h"
#include "sv.h"
#include <app/buffer.h>
#include <app/c.h>
#include <app/eddy.h>
#include <app/editor.h>
#include <app/listbox.h>
#include <app/widget.h>
#include <base/process.h>
#include <lsp/lsp.h>
#include <lsp/schema/CompletionItem.h>

// -- C Mode ----------------------------------------------------------------

MODE_CLASS_DEF(CMode, c_mode);

void completions_submit(ListBox *listbox, ListBoxEntry selection)
{
    CompletionItem *item = (CompletionItem *) selection.payload;
    Editor         *editor = eddy.editor;
    BufferView     *view = editor->buffers.elements + editor->current_buffer;
    Buffer         *buffer = eddy.buffers.elements + view->buffer_num;
    switch (item->textEdit.tag) {
    case 0: {
        TextEdit   edit = item->textEdit._0;
        IntVector2 start = { edit.range.start.character, edit.range.start.line };
        IntVector2 end = { edit.range.end.character, edit.range.end.line };
        size_t     offset = buffer_position_to_index(buffer, start);
        size_t     length = buffer_position_to_index(buffer, end) - offset;
        size_t     prefix_length = view->cursor - offset;

        if (length == 0 || prefix_length == 0) {
            buffer_insert(buffer, edit.newText, offset);
            view->new_cursor = offset + edit.newText.length;
        } else {
            buffer_replace(buffer, offset, length, edit.newText);
            view->new_cursor = offset + length;
        }
        view->cursor_col = -1;
    } break;
    case 1:
    default:
        UNREACHABLE();
    }
}

void completions_draw(ListBox *completions)
{
    widget_draw_rectangle(completions, 0.0, 0.0, 0.0, 0.0, DARKGRAY);
    widget_draw_outline(completions, 2, 2, -2.0, -2.0, RAYWHITE);
    listbox_draw_entries(completions, 4);
}

void c_mode_cmd_completion(CommandContext *ctx)
{
    Editor                 *editor = (Editor *) ctx->target->parent->parent;
    BufferView             *view = editor->buffers.elements + editor->current_buffer;
    OptionalCompletionItems items_maybe = lsp_request_completions(view);
    if (!items_maybe.has_value) {
        return;
    }
    CompletionItems items = items_maybe.value;

    ListBox *listbox = widget_new(ListBox);
    listbox->textsize = 0.75;

    int col = view->cursor_pos.x - view->left_column;
    int line = view->cursor_pos.y - view->top_line;
    listbox->viewport.x = editor->viewport.x + col * eddy.cell.x;
    listbox->viewport.y = editor->viewport.y + (line + 1) * eddy.cell.y;
    listbox->viewport.width = 300;
    listbox->viewport.height = 150;
    listbox->lines = (listbox->viewport.height - 8) / (eddy.cell.y * listbox->textsize + 2);
    listbox->viewport.height = 8 + listbox->lines * (eddy.cell.y * listbox->textsize + 2);
    listbox->handlers.draw = (WidgetDraw) completions_draw;
    listbox->handlers.resize = NULL;

    listbox->submit = completions_submit;
    for (size_t ix = 0; ix < items.size; ++ix) {
        CompletionItem *item = da_element_CompletionItem(&items, ix);
        da_append_ListBoxEntry(&listbox->entries, (ListBoxEntry) { item->label, item });
    }
    listbox_show(listbox);
}

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

int indent_for_line(Buffer *buffer, size_t lineno)
{
    while (lineno > 0) {
        --lineno;
        Index *l = buffer->lines.elements + lineno;
        int    non_space;
        int    indent = 0;
        for (non_space = 0; non_space < l->line.length && isspace(l->line.ptr[non_space]); ++non_space) {
            switch (l->line.ptr[non_space]) {
            case '\t':
                indent = ((indent / 4) + 1) * 4;
                break;
            default:
                ++indent;
            }
        }
        if (non_space < l->line.length) {
            int last_non_space;
            for (last_non_space = l->line.length; last_non_space >= 0 && isspace(l->line.ptr[last_non_space]); --last_non_space)
                ;
            if (l->line.ptr[last_non_space] == '{') {
                indent += 4;
            }
            return indent;
        }
        lineno--;
    }
    return 0;
}

void c_mode_cmd_split_line(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target->parent->parent;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    size_t      lineno = buffer_line_for_index(buffer, view->new_cursor);
    int         indent_this_line = indent_for_line(buffer, lineno);
    int         indent_new_line = indent_this_line;
    Index      *l = buffer->lines.elements + lineno;
    int         first_non_space, last_non_space;

    // | | | | |x| |=| |1|0|;| | |\n|
    // |}| | |\n|
    //  0 1 2 3 4 5 6 7 8 9 0 1 2 3
    //                          ^
    for (first_non_space = l->index_of; buffer->text.ptr[first_non_space] != 0 && isspace(buffer->text.ptr[first_non_space]); ++first_non_space)
        ;
    for (last_non_space = view->new_cursor - 1; last_non_space >= l->index_of && isspace(buffer->text.ptr[last_non_space]); --last_non_space)
        ;
    size_t text_length = last_non_space - first_non_space + 1;
    bool   last_is_close_curly = buffer->text.ptr[last_non_space] == '}';
    bool   last_is_open_curly = buffer->text.ptr[last_non_space] == '{';

    // Remove trailing whitespace:
    if (view->new_cursor > last_non_space) {
        // Actually strip the trailing whitespace:
        buffer_delete(buffer, last_non_space + 1, view->new_cursor - last_non_space - 1);
    }

    // Reindent:
    if (first_non_space > l->index_of) {
        buffer_delete(buffer, l->index_of, first_non_space - l->index_of);
    }
    if (last_is_close_curly) {
        indent_this_line -= 4;
        indent_new_line = indent_this_line;
    }
    if (indent_this_line > 0) {
        StringView s = sv_printf("%*s", indent_this_line, "");
        buffer_insert(buffer, s, l->index_of);
        sv_free(s);
    }
    if (last_is_open_curly) {
        indent_new_line += 4;
    }

    StringView s = sv_printf("\n%*s", indent_new_line, "");
    buffer_insert(buffer, s, l->index_of + indent_this_line + text_length);
    sv_free(s);
    view->new_cursor = l->index_of + indent_this_line + text_length + 1 + indent_new_line;
    view->cursor_col = -1;
}

void c_mode_cmd_indent(CommandContext *ctx)
{
    Editor     *editor = (Editor *) ctx->target->parent->parent;
    BufferView *view = editor->buffers.elements + editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    size_t      lineno = buffer_line_for_index(buffer, view->new_cursor);
    int         indent = indent_for_line(buffer, lineno);
    Index      *l = buffer->lines.elements + lineno;
    int         first_non_space, last_non_space;

    // | | | | |x| |=| |1|0|;| | |\n|
    // |}| | |\n|
    //  0 1 2 3 4 5 6 7 8 9 0 1 2 3
    //                          ^
    for (first_non_space = l->index_of; buffer->text.ptr[first_non_space] != 0 && isspace(buffer->text.ptr[first_non_space]); ++first_non_space)
        ;
    for (last_non_space = l->index_of + l->line.length - 1; last_non_space >= l->index_of && isspace(buffer->text.ptr[last_non_space]); --last_non_space)
        ;
    size_t text_length = last_non_space - first_non_space + 1;
    bool   last_is_closing_curly = buffer->text.ptr[last_non_space] == '}';

    // Remove trailing whitespace:
    if (l->index_of + l->line.length - 1 > last_non_space) {
        // Actually strip the trailing whitespace:
        buffer_delete(buffer, last_non_space + 1, view->new_cursor - last_non_space - 1);
    }

    // Reindent:
    if (first_non_space > l->index_of) {
        buffer_delete(buffer, l->index_of, first_non_space - l->index_of);
    }
    if (last_is_closing_curly) {
        indent -= 4;
    }
    if (indent > 0) {
        StringView s = sv_printf("%*s", indent, "");
        buffer_insert(buffer, s, l->index_of);
        sv_free(s);
    }
    view->new_cursor = l->index_of + indent;
}

void c_mode_cmd_unindent(CommandContext *ctx)
{
    // Not sure how this is supposed to work
}

bool c_mode_character(CMode *mode, int ch)
{
    return false;
}

void c_mode_init(CMode *mode)
{
    widget_add_command(mode, sv_from("c-format-source"), c_mode_cmd_format_source,
        (KeyCombo) { KEY_L, KMOD_SHIFT | KMOD_CONTROL });
    widget_add_command(mode, sv_from("c-show-completions"), c_mode_cmd_completion,
        (KeyCombo) { KEY_SPACE, KMOD_CONTROL });
    widget_add_command(mode, sv_from("c-split-line"), c_mode_cmd_split_line,
        (KeyCombo) { KEY_ENTER, KMOD_NONE }, (KeyCombo) { KEY_KP_ENTER, KMOD_NONE });
    widget_add_command(mode, sv_from("c-indent"), c_mode_cmd_indent,
        (KeyCombo) { KEY_TAB, KMOD_NONE });
    widget_add_command(mode, sv_from("c-unindent"), c_mode_cmd_unindent,
        (KeyCombo) { KEY_TAB, KMOD_SHIFT });
    // mode->handlers.on_draw = (WidgetOnDraw) c_mode_on_draw;
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
        ++ix;
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
