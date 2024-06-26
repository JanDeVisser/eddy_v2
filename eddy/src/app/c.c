/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <app/buffer.h>
#include <app/c.h>
#include <app/eddy.h>
#include <app/editor.h>
#include <app/listbox.h>
#include <app/widget.h>
#include <lsp/schema/CompletionList.h>
#include <lsp/schema/CompletionParams.h>
#include <lsp/schema/DocumentFormattingParams.h>
#include <lsp/schema/DocumentRangeFormattingParams.h>

#define C_KEYWORDS(S) \
    S(alignas)        \
    S(alignof)        \
    S(auto)           \
    S(bool)           \
    S(break)          \
    S(case)           \
    S(char)           \
    S(const)          \
    S(constexpr)      \
    S(continue)       \
    S(default)        \
    S(do)             \
    S(double)         \
    S(else)           \
    S(enum)           \
    S(extern)         \
    S(false)          \
    S(float)          \
    S(for)            \
    S(goto)           \
    S(if)             \
    S(inline)         \
    S(int)            \
    S(long)           \
    S(nullptr)        \
    S(register)       \
    S(restrict)       \
    S(return)         \
    S(short)          \
    S(signed)         \
    S(sizeof)         \
    S(static)         \
    S(static_assert)  \
    S(struct)         \
    S(switch)         \
    S(thread_local)   \
    S(true)           \
    S(typedef)        \
    S(typeof)         \
    S(typeof_unqual)  \
    S(union)          \
    S(unsigned)       \
    S(void)           \
    S(volatile)       \
    S(while)          \
    S(_Alignas)       \
    S(_Alignof)       \
    S(_Atomic)        \
    S(_BitInt)        \
    S(_Bool)          \
    S(_Complex)       \
    S(_Decimal128)    \
    S(_Decimal32)     \
    S(_Decimal64)     \
    S(_Generic)       \
    S(_Imaginary)     \
    S(_Noreturn)      \
    S(_Static_assert) \
    S(_Thread_local)

static Process *clangd_start(LSP *lsp);

LSPHandlers c_lsp_handlers = {
    .start = clangd_start,
};

MODE_CLASS_DEF(CMode, c_mode);
MODE_DATA_CLASS_DEF(CModeData, c_mode_data);

#define C_KEYWORDS(S)   \
    S(alignas)        \
    S(alignof)        \
    S(auto)           \
    S(bool)           \
    S(break)          \
    S(case)           \
    S(char)           \
    S(const)          \
    S(constexpr)      \
    S(continue)       \
    S(default)        \
    S(do)             \
    S(double)         \
    S(else)           \
    S(enum)           \
    S(extern)         \
    S(false)          \
    S(float)          \
    S(for)            \
    S(goto)           \
    S(if)             \
    S(inline)         \
    S(int)            \
    S(long)           \
    S(nullptr)        \
    S(register)       \
    S(restrict)       \
    S(return)         \
    S(short)          \
    S(signed)         \
    S(sizeof)         \
    S(static)         \
    S(static_assert)  \
    S(struct)         \
    S(switch)         \
    S(thread_local)   \
    S(true)           \
    S(typedef)        \
    S(typeof)         \
    S(typeof_unqual)  \
    S(union)          \
    S(unsigned)       \
    S(void)           \
    S(volatile)       \
    S(while)          \
    S(_Alignas)       \
    S(_Alignof)       \
    S(_Atomic)        \
    S(_BitInt)        \
    S(_Bool)          \
    S(_Complex)       \
    S(_Decimal128)    \
    S(_Decimal32)     \
    S(_Decimal64)     \
    S(_Generic)       \
    S(_Imaginary)     \
    S(_Noreturn)      \
    S(_Static_assert) \
    S(_Thread_local)

// -- C Mode ----------------------------------------------------------------

Process *clangd_start(LSP *lsp)
{
    Process *ret = process_create(SV("clangd"), "--use-dirty-headers", "--background-index");
    ret->stderr_file = SV("/tmp/clangd.log");
    return ret;
}

int handle_c_directive(Lexer *lexer, int directive);

typedef enum {
    CDirectiveElse,
    CDirectiveElif,
    CDirectiveElifdef,
    CDirectiveElifndef,
    CDirectiveEndif,
    CDirectiveError,
    CDirectiveDefine,
    CDirectiveIfdef,
    CDirectiveIfndef,
    CDirectiveIf,
    CDirectiveInclude,
    CDirectiveMax,
} CDirective;

static char const *c_directives[CDirectiveMax + 1] = {
    [CDirectiveElse] = "else",
    [CDirectiveElif] = "elif",
    [CDirectiveElifdef] = "elifdef",
    [CDirectiveElifndef] = "elifndef",
    [CDirectiveEndif] = "endif",
    [CDirectiveError] = "error",
    [CDirectiveDefine] = "define",
    [CDirectiveIfdef] = "ifdef",
    [CDirectiveIfndef] = "ifndef",
    [CDirectiveIf] = "if",
    [CDirectiveInclude] = "include",
    [CDirectiveMax] = NULL,
};

static Keyword c_keywords[] = {
#undef S
#define S(kw) { .keyword = #kw, .code = __COUNTER__ },
    C_KEYWORDS(S)
#undef S
        { NULL, 0 },
};

static Language c_language = {
    .name = (StringView) { .ptr = "C", .length = 1 },
    .directives = c_directives,
    .preprocessor_trigger = (Token) { .symbol = '#', .kind = TK_SYMBOL },
    .keywords = c_keywords,
    .directive_handler = handle_c_directive,
};

void c_mode_on_draw(CMode *mode)
{
}

void c_mode_buffer_event_listener(Buffer *buffer, BufferEvent event)
{
    switch (event.type) {
    case ETInsert:
        lsp_did_change(buffer, event.range.start, event.range.end, buffer_sv_from_ref(buffer, event.insert.text));
        break;
    case ETDelete:
        lsp_did_change(buffer, event.range.start, event.range.end, sv_null());
        break;
    case ETReplace:
        lsp_did_change(buffer, event.range.start, event.range.end, buffer_sv_from_ref(buffer, event.replace.replacement));
        break;
    case ETIndexed:
        lsp_semantic_tokens(buffer);
        break;
    case ETSave:
        lsp_did_save(buffer);
        break;
    case ETClose:
        lsp_did_close(buffer);
        break;
    default:
        break;
    }
}

static void completions_submit(ListBox *listbox, ListBoxEntry selection)
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

static void completions_draw(ListBox *completions)
{
    widget_draw_rectangle(completions, 0.0f, 0.0f, 0.0f, 0.0f, colour_to_color(eddy.theme.editor.bg));
    widget_draw_outline(completions, 2, 2, -2.0f, -2.0f, colour_to_color(eddy.theme.editor.fg));
    listbox_draw_entries(completions, 4);
}

static void fill_completions_list(ListBox *listbox, JSONValue resp)
{
    Response response = response_decode(&resp);
    if (response_error(&response)) {
        eddy_set_message(&eddy, "textDocument/completion LSP message returned error");
        return;
    }

    OptionalJSONValue result = response.result;
    CompletionItems   items = { 0 };
    switch (result.value.type) {
    case JSON_TYPE_NULL:
        eddy_set_message(&eddy, "textDocument/completion LSP message returned null");
        return;
    case JSON_TYPE_ARRAY: {
        OptionalCompletionItems items_maybe = CompletionItems_decode(result);
        assert(items_maybe.has_value);
        items = items_maybe.value;
    } break;
    case JSON_TYPE_OBJECT: {
        OptionalCompletionList completionlist_maybe = CompletionList_decode(response.result);
        assert(completionlist_maybe.has_value);
        items = completionlist_maybe.value.items;
    } break;
    default:
        UNREACHABLE();
    }
    if (items.size == 0) {
        eddy_set_message(&eddy, "textDocument/completion LSP message returned no results");
        return;
    }
    for (size_t ix = 0; ix < items.size; ++ix) {
        CompletionItem *item = da_element_CompletionItem(&items, ix);
        da_append_ListBoxEntry(&listbox->entries, (ListBoxEntry) { .text = item->label, .payload = item });
    }
    listbox_show(listbox);
}

void c_mode_cmd_completion(CMode *mode, JSONValue unused)
{
    BufferView *view = (BufferView *) mode->parent;
    Editor     *editor = (Editor *) view->parent;

    lsp_initialize(&mode->lsp);
    Buffer *buffer = eddy.buffers.elements + view->buffer_num;

    if (sv_empty(buffer->name)) {
        return;
    }

    ListBox *listbox = widget_new(ListBox);
    listbox->textsize = 0.75f;

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
    widget_register(listbox, "lsp-textDocument/completion", (WidgetCommandHandler) fill_completions_list);
    listbox->submit = completions_submit;

    CompletionParams completion_params = { 0 };
    completion_params.textDocument = (TextDocumentIdentifier) {
        .uri = buffer_uri(buffer),
    };
    completion_params.position = (Position) {
        .line = view->cursor_pos.line,
        .character = view->cursor_pos.column
    };

    OptionalJSONValue params_json = CompletionParams_encode(completion_params);
    MUST(Int, lsp_message(&mode->lsp, listbox, "textDocument/completion", params_json));
}

void c_mode_formatting_response(CMode *mode, JSONValue resp)
{
    Response response = response_decode(&resp);
    if (response_error(&response)) {
        return;
    }
    if (response.result.value.type != JSON_TYPE_ARRAY) {
        return;
    }

    BufferView *view = (BufferView *) mode->parent;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    TextEdits   edits = MUST_OPTIONAL(TextEdits, TextEdits_decode(response.result));
    for (size_t ix = 0; ix < edits.size; ++ix) {
        TextEdit   edit = edits.elements[ix];
        IntVector2 start = { edit.range.start.character, edit.range.start.line };
        IntVector2 end = { edit.range.end.character, edit.range.end.line };
        size_t     offset = buffer_position_to_index(buffer, start);
        size_t     length = buffer_position_to_index(buffer, end) - offset;

        if (sv_empty(edit.newText)) {
            buffer_delete(buffer, offset, length);
        } else if (length == 0) {
            buffer_insert(buffer, edit.newText, offset);
        } else {
            buffer_replace(buffer, offset, length, edit.newText);
        }
    }
    if (edits.size >= 0) {
        eddy_set_message(&eddy, "Formatted. Made %d changes", edits.size);
    }
    da_free_TextEdit(&edits);
}

void c_mode_cmd_format_source(CMode *mode, JSONValue unused)
{
    BufferView *view = (BufferView *) mode->parent;
    Buffer     *buffer = (Buffer *) eddy.buffers.elements + view->buffer_num;
    lsp_initialize(&mode->lsp);

    if (sv_empty(buffer->name)) {
        return;
    }

    DocumentFormattingParams params = { 0 };
    params.textDocument.uri = buffer_uri(buffer);
    params.options.insertSpaces = true;
    params.options.tabSize = 4;
    params.options.trimTrailingWhitespace = (OptionalBool) { .has_value = true, .value = true };
    params.options.insertFinalNewline = (OptionalBool) { .has_value = true, .value = true };
    params.options.trimFinalNewlines = (OptionalBool) { .has_value = true, .value = true };

    OptionalJSONValue params_json = DocumentFormattingParams_encode(params);
    MUST(Int, lsp_message(&mode->lsp, mode, "textDocument/formatting", params_json));
}

void c_mode_cmd_format_selection(CMode *mode, JSONValue unused)
{
    Editor     *editor = (Editor *) mode->parent->parent;
    BufferView *view = (BufferView *) mode->parent;
    Buffer     *buffer = (Buffer *) eddy.buffers.elements + view->buffer_num;
    lsp_initialize(&mode->lsp);

    if (sv_empty(buffer->name)) {
        return;
    }

    Range r = { 0 };
    if (view->selection == -1) {
        r.start.line = buffer_line_for_index(buffer, (int) view->new_cursor);
        r.end.line = r.start.line + 1;
    } else {
        int selection_start = imin((int) view->selection, (int) view->new_cursor);
        int selection_end = imax((int) view->selection, (int) view->new_cursor);
        r.start.line = buffer_line_for_index(buffer, selection_start);
        r.start.character = selection_start - buffer->lines.elements[r.start.line].index_of;
        r.end.line = buffer_line_for_index(buffer, selection_end);
        r.end.character = selection_end - buffer->lines.elements[r.end.line].index_of;
    }

    DocumentRangeFormattingParams params = { 0 };

    params.textDocument.uri = buffer_uri(buffer);
    params.range = r;
    params.options.insertSpaces = true;
    params.options.tabSize = 4;
    params.options.trimTrailingWhitespace = (OptionalBool) { .has_value = true, .value = true };
    params.options.insertFinalNewline = (OptionalBool) { .has_value = true, .value = true };
    params.options.trimFinalNewlines = (OptionalBool) { .has_value = true, .value = true };

    OptionalJSONValue params_json = DocumentRangeFormattingParams_encode(params);
    MUST(Int, lsp_message(&mode->lsp, mode, "textDocument/rangeFormatting", params_json));
}

Widget * cmode_make_cmode_data(CMode *mode)
{
    return (Widget *) widget_new(CModeData);
}

void c_mode_init(CMode *mode)
{
    mode->data_widget_factory = (WidgetFactory) cmode_make_cmode_data;
    mode->event_listener = c_mode_buffer_event_listener;
    mode->filetypes = sv_split(SV(".c;.cpp;.h;.hpp;.c++"), SV(";"));
    mode->lsp = (LSP) {
        .handlers = c_lsp_handlers,
    };
    widget_add_command(mode, "c-format-source", (WidgetCommandHandler) c_mode_cmd_format_source,
        (KeyCombo) { KEY_L, KMOD_SHIFT | KMOD_CONTROL });
    widget_add_command(mode, "c-show-completions", (WidgetCommandHandler) c_mode_cmd_completion,
        (KeyCombo) { KEY_SPACE, KMOD_CONTROL });
    widget_add_command(mode, "c-split-line", (WidgetCommandHandler) mode_cmd_split_line,
        (KeyCombo) { KEY_ENTER, KMOD_NONE }, (KeyCombo) { KEY_KP_ENTER, KMOD_NONE });
    widget_add_command(mode, "c-indent", (WidgetCommandHandler) c_mode_cmd_format_selection,
        (KeyCombo) { KEY_TAB, KMOD_NONE });
    widget_add_command(mode, "c-unindent", (WidgetCommandHandler) mode_cmd_unindent,
        (KeyCombo) { KEY_TAB, KMOD_SHIFT });
    widget_register(mode, "lsp-textDocument/formatting", (WidgetCommandHandler) c_mode_formatting_response);
    widget_register(mode, "lsp-textDocument/rangeFormatting", (WidgetCommandHandler) c_mode_formatting_response);
    mode->language = &c_language;
    // mode->handlers.on_draw = (WidgetOnDraw) c_mode_on_draw;
}

void c_mode_data_init(CModeData *mode)
{
    //
}


typedef enum {
    CDirectiveStateInit = 0,
    CDirectiveStateIncludeQuote,
    CDirectiveStateMacroName,
} CDirectiveState;

static int handle_include_directive(Lexer *lexer)
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
            lexer_set_current(lexer, (Token) { .kind = TK_WHITESPACE, .text = { buffer, ix } });
            return CDirectiveInclude;
        }
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
            ++ix;
        }
        lexer_set_current(lexer, (Token) { .kind = TK_DIRECTIVE_ARG, .text = { buffer, ix } });
    } break;
    default:
        UNREACHABLE();
    }
    lexer->language_data = NULL;
    return NO_DIRECTIVE;
}

static int handle_macro_name_directive(Lexer *lexer)
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
            lexer_set_current(lexer, (Token) { .kind = TK_WHITESPACE, .text = { buffer, ix } });
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
