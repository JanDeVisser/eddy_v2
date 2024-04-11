/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <ctype.h>

#include <app/buffer.h>
#include <app/c.h>
#include <app/eddy.h>
#include <app/listbox.h>
#include <app/theme.h>
#include <base/io.h>
#include <lsp/lsp.h>
#include <lsp/schema/DidChangeTextDocumentParams.h>
#include <lsp/schema/DidCloseTextDocumentParams.h>
#include <lsp/schema/DidOpenTextDocumentParams.h>
#include <lsp/schema/DidSaveTextDocumentParams.h>
#include <lsp/schema/SemanticTokens.h>
#include <lsp/schema/SemanticTokensParams.h>

DA_IMPL(Index);
DA_IMPL(DisplayToken);
DA_IMPL(Buffer);

DA_IMPL(BufferEvent);

SIMPLE_WIDGET_CLASS_DEF(Buffer, buffer);

void buffer_semantic_tokens_response(Buffer *buffer, JSONValue resp);
void buffer_apply(Buffer *buffer, BufferEvent event);

void buffer_init(Buffer *buffer)
{
    widget_register(
        buffer,
        "lsp-textDocument/semanticTokens/full",
        (WidgetCommandHandler) buffer_semantic_tokens_response);
    buffer->listeners = NULL;
}

ErrorOrBuffer buffer_open(Buffer *buffer, StringView name)
{
    buffer->name = sv_copy(name);
    buffer->text.view = TRY_TO(StringView, Buffer, read_file_by_name(name));
    buffer->lines.size = 0;
    buffer_build_indices(buffer);
    RETURN(Buffer, buffer);
}

void buffer_close(Buffer *buffer)
{
    BufferEvent event = { 0 };
    event.type = ETClose;
    buffer_apply(buffer, event);
}

void buffer_build_indices(Buffer *buffer)
{
    assert(buffer->indexed_version <= buffer->version);
    trace(EDIT, "buffer_build_indices('%.*s')", SV_ARG(buffer->name));
    if (buffer->indexed_version == buffer->version && buffer->lines.size > 0) {
        trace(EDIT, "buffer_build_indices('%.*s'): clean. indexed_version = %zu version = %zu lines = %zu",
            SV_ARG(buffer->name), buffer->indexed_version, buffer->version, buffer->lines.size);
        return;
    }
    buffer->lines.size = 0;
    buffer->tokens.size = 0;
    Editor *editor = eddy.editor;
    Lexer lexer = {0};
    for (size_t ix = 0; ix < editor->buffers.size; ++ix) {
        BufferView *view = editor->buffers.elements + ix;
        if (view->buffer_num == buffer->buffer_ix && view->mode != NULL) {
            lexer = lexer_for_language(((Mode *) view->mode)->language);
        }
    }
    if (lexer.language == NULL) {
        trace(EDIT, "buffer_build_indices('%.*s'): no view found", SV_ARG(buffer->name));
        buffer->indexed_version = buffer->version;
        return;
    }
    lexer.whitespace_significant = true;
    lexer.include_comments = true;
    lexer_push_source(&lexer, buffer->text.view, buffer->name);
    Index *current = da_append_Index(&buffer->lines, (Index) { 0, buffer->text.view });
    size_t lineno = 0;
    trace(EDIT, "Buffer size: %zu", buffer->text.view.length);
    trace_nonl(EDIT, "%5zu: ", lineno);
    int dix = 0;
    current->first_diagnostic = 0;
    current->num_diagnostics = 0;
    if (dix < buffer->diagnostics.size) {
        current->first_diagnostic = dix;
        while (dix < buffer->diagnostics.size && buffer->diagnostics.elements[dix].range.start.line == 0) {
            ++current->num_diagnostics;
            ++dix;
        }
    }
    while (true) {
        Token t = lexer_lex(&lexer);
        if (token_matches_kind(t, TK_END_OF_LINE) || token_matches_kind(t, TK_END_OF_FILE)) {
            if (current->num_tokens == 0) {
                trace(EDIT, "[EOL]");
            } else {
                trace(EDIT, "[EOL] %zu..%zu", current->first_token, current->first_token + current->num_tokens - 1);
            }
            current->line.length = t.location.index - current->index_of;
            if (t.kind == TK_END_OF_FILE) {
                break;
            }
            ++lineno;
            current = da_append_Index(&buffer->lines, (Index) { t.location.index + 1, { buffer->text.view.ptr + t.location.index + 1, 0 }, 0, 0 });
            current->first_diagnostic = 0;
            current->num_diagnostics = 0;
            if (dix < buffer->diagnostics.size) {
                current->first_diagnostic = dix;
                while (dix < buffer->diagnostics.size && buffer->diagnostics.elements[dix].range.start.line == lineno) {
                    ++current->num_diagnostics;
                    ++dix;
                }
            }
            trace_nonl(EDIT, "%5zu: ", lineno);
            continue;
        }
        OptionalColours colours = theme_token_colours(&eddy.theme, t);
        Colour          colour = colours.has_value ? colours.value.fg : eddy.theme.editor.fg;
        if (token_matches_kind(t, TK_WHITESPACE)) {
            trace_nonl(EDIT, "%*.s", (int) t.text.length, "");
        } else {
            StringView s = colour_to_rgb(colour);
            trace_nonl(EDIT, "[%.*s %.*s %.*s]", SV_ARG(t.text), SV_ARG(TokenKind_name(t.kind)), SV_ARG(s));
            sv_free(s);
        }
        if (current->num_tokens == 0) {
            current->first_token = buffer->tokens.size;
        }
        ++current->num_tokens;
        da_append_DisplayToken(
            &buffer->tokens,
            (DisplayToken) {
                t.location.index,
                t.text.length,
                lineno,
                colour_to_color(colour),
            });
    }
    trace_nl(EDIT);
    trace(EDIT, "[EOF]");
    trace(EDIT, "=====================");
    buffer->indexed_version = buffer->version;
    BufferEvent event = { .type = ETIndexed };
    for (BufferEventListenerList *list_entry = buffer->listeners; list_entry != NULL; list_entry = list_entry->next) {
        list_entry->listener(buffer, event);
    }
}

size_t buffer_line_for_index(Buffer *buffer, int index)
{
    assert(buffer != NULL);
    Indices *indices = &buffer->lines;
    if (indices->size == 0) {
        return 0;
    }
    size_t line_min = 0;
    size_t line_max = indices->size - 1;
    while (true) {
        size_t line = line_min + (line_max - line_min) / 2;
        if ((line < indices->size - 1 && indices->elements[line].index_of <= index && index < indices->elements[line + 1].index_of) || (line == indices->size - 1 && indices->elements[line].index_of <= index)) {
            return line;
        }
        if (indices->elements[line].index_of > index) {
            line_max = line;
        } else {
            line_min = line + 1;
        }
    }
}

IntVector2 buffer_index_to_position(Buffer *buffer, int index)
{
    IntVector2 ret = { 0 };
    ret.line = buffer_line_for_index(buffer, index);
    ret.column = index - buffer->lines.elements[ret.line].index_of;
    return ret;
}

size_t buffer_position_to_index(Buffer *buffer, IntVector2 position)
{
    Index *line = buffer->lines.elements + position.line;
    return line->index_of + position.column;
}

StringRef append_to_undo_buffer(Buffer *buffer, StringView sv)
{
    if (sv.ptr < buffer->undo_buffer.view.ptr || sv.ptr > buffer->undo_buffer.view.ptr + buffer->undo_buffer.view.length) {
        size_t buffer_pos = buffer->undo_buffer.view.length;
        sb_append_sv(&buffer->undo_buffer, sv);
        return (StringRef) { buffer_pos, sv.length };
    }
    return (StringRef) { sv.ptr - buffer->undo_buffer.view.ptr, sv.length };
}

StringRef append_to_undo_buffer_from_text(Buffer *buffer, size_t at, size_t count)
{
    return append_to_undo_buffer(buffer, (StringView) { buffer->text.view.ptr + at, count });
}

StringView buffer_sv_from_ref(Buffer *buffer, StringRef ref)
{
    if (ref.length == 0) {
        return sv_null();
    }
    return (StringView) { buffer->undo_buffer.view.ptr + ref.index, ref.length };
}

void buffer_apply(Buffer *buffer, BufferEvent event)
{
    switch (event.type) {
    case ETInsert: {
        if (event.insert.text.length == 0) {
            return;
        }
        event.range.start = buffer_index_to_position(buffer, event.position);
        event.range.end = event.range.start;
        StringView sv = buffer_sv_from_ref(buffer, event.insert.text);
        if (event.position < buffer->text.view.length) {
            sb_insert_sv(&buffer->text, sv, event.position);
        } else {
            sb_append_sv(&buffer->text, sv);
        }
        ++buffer->version;
    } break;
    case ETDelete: {
        if (event.delete.count == 0) {
            return;
        }
        event.range.start = buffer_index_to_position(buffer, event.position);
        event.range.end = buffer_index_to_position(buffer, event.position + event.delete.count);
        sb_remove(&buffer->text, event.position, event.delete.count);
        ++buffer->version;
    } break;
    case ETReplace: {
        if (event.replace.replacement.length == 0) {
            return;
        }
        event.range.start = buffer_index_to_position(buffer, event.position);
        event.range.end = buffer_index_to_position(buffer, event.position + event.replace.overwritten.length);
        sb_remove(&buffer->text, event.position, event.replace.overwritten.length);
        StringView sv = buffer_sv_from_ref(buffer, event.replace.replacement);
        if (event.position < buffer->text.view.length) {
            sb_insert_sv(&buffer->text, sv, event.position);
        } else {
            sb_append_sv(&buffer->text, sv);
        }
        ++buffer->version;
    } break;
    case ETSave: {
        assert(buffer->saved_version <= buffer->version);
        StringView name = buffer_sv_from_ref(buffer, event.save.file_name);
        if (sv_empty(name) && buffer->saved_version == buffer->version) {
            return;
        }
        if (sv_not_empty(name)) {
            buffer->name = sv_copy(name);
            sv_free(buffer->uri);
        }
        if (sv_empty(buffer->name)) {
            return;
        }
        MUST(Size, write_file_by_name(buffer->name, buffer->text.view));
        buffer->saved_version = buffer->undo_stack.size;
    } break;
    case ETClose: {
        for (BufferEventListenerList *list_entry = buffer->listeners; list_entry != NULL; list_entry = list_entry->next) {
            list_entry->listener(buffer, event);
        }
        sv_free(buffer->text.view);
        sv_free(buffer->undo_buffer.view);
        sv_free(buffer->name);
        sv_free(buffer->uri);
        da_free_BufferEvent(&buffer->undo_stack);
        da_free_DisplayToken(&buffer->tokens);
        da_free_Index(&buffer->lines);
        for (BufferEventListenerList *entry = buffer->listeners; entry;) {
            BufferEventListenerList *next = entry->next;
            free(entry);
            entry = next;
        }
        memset(buffer, 0, sizeof(Buffer));
        return;
    } break;
    default:
        break;
    }
    for (BufferEventListenerList *list_entry = buffer->listeners; list_entry != NULL; list_entry = list_entry->next) {
        list_entry->listener(buffer, event);
    }
}

void buffer_edit(Buffer *buffer, BufferEvent event)
{
    switch (event.type) {
    case ETInsert: {
        if (event.insert.text.length == 0) {
            return;
        }
        event.position = iclamp(event.position, 0, buffer->text.view.length);
    } break;
    case ETDelete: {
        event.position = iclamp(event.position, 0, buffer->text.view.length);
        event.delete.count = iclamp(event.delete.count, 0, buffer->text.view.length - event.position);
        if (event.delete.count == 0) {
            return;
        }
        event.delete.deleted = append_to_undo_buffer_from_text(buffer, event.position, event.delete.count);
    } break;
    case ETReplace: {
        event.position = iclamp(event.position, 0, buffer->text.view.length);
        int count = iclamp(event.replace.overwritten.length, 0, buffer->text.view.length - event.position);
        if (count <= 0) {
            return;
        }
        event.replace.overwritten = append_to_undo_buffer_from_text(buffer, event.position, count);
    } break;
    default:
        break;
    }
    buffer_apply(buffer, event);
    da_append_BufferEvent(&buffer->undo_stack, event);
    buffer->undo_pointer = buffer->undo_stack.size;
}

BufferEvent revert_edit(BufferEvent event)
{
    BufferEvent ret = { 0 };
    switch (event.type) {
    case ETInsert:
        ret.type = ETDelete;
        ret.position = event.position;
        ret.delete.count = event.insert.text.length;
        break;
    case ETDelete:
        ret.type = ETInsert;
        ret.position = event.position;
        ret.insert.text = event.delete.deleted;
        break;
    case ETReplace:
        ret.type = ETReplace;
        ret.position = event.position;
        ret.replace.overwritten = event.replace.replacement;
        ret.replace.replacement = event.replace.overwritten;
        break;
    default:
        break;
    }
    return ret;
}

void buffer_undo(Buffer *buffer)
{
    if (!buffer->undo_pointer) {
        return;
    }
    buffer_apply(buffer, revert_edit(buffer->undo_stack.elements[--buffer->undo_pointer]));
}

void buffer_redo(Buffer *buffer)
{
    if (buffer->undo_pointer >= buffer->undo_stack.size) {
        return;
    }
    buffer_apply(buffer, revert_edit(buffer->undo_stack.elements[buffer->undo_pointer++]));
}

void buffer_insert(Buffer *buffer, StringView text, int pos)
{
    BufferEvent event = { 0 };
    event.type = ETInsert;
    event.position = pos;
    event.insert.text = append_to_undo_buffer(buffer, text);
    buffer_edit(buffer, event);
}

void buffer_delete(Buffer *buffer, size_t at, size_t count)
{
    BufferEvent event = { 0 };
    event.type = ETDelete;
    event.position = at;
    event.delete.count = count;
    buffer_edit(buffer, event);
}

void buffer_replace(Buffer *buffer, size_t at, size_t num, StringView replacement)
{
    BufferEvent event = { 0 };
    event.type = ETReplace;
    event.position = at;
    event.replace.replacement = append_to_undo_buffer(buffer, replacement);
    event.replace.overwritten.length = num;
    buffer_edit(buffer, event);
}

void buffer_merge_lines(Buffer *buffer, int top_line)
{
    if (top_line > buffer->lines.size - 1) {
        return;
    }
    if (top_line < 0) {
        top_line = 0;
    }
    Index line = buffer->lines.elements[top_line];
    buffer_replace(buffer, line.index_of + line.line.length, 1, sv_from(" "));
}

void buffer_save(Buffer *buffer)
{
    BufferEvent event = { 0 };
    event.type = ETSave;
    event.position = 0;
    buffer_apply(buffer, event);
}

void buffer_save_as(Buffer *buffer, StringView name)
{
    BufferEvent event = { 0 };
    event.type = ETSave;
    event.position = 0;
    event.save.file_name = append_to_undo_buffer(buffer, name);
    buffer_apply(buffer, event);
}

size_t buffer_word_boundary_left(Buffer *buffer, size_t index)
{
    if (isalnum(buffer->text.view.ptr[index]) || buffer->text.view.ptr[index] == '_') {
        while (((int) index) > 0 && (isalnum(buffer->text.view.ptr[index]) || buffer->text.view.ptr[index] == '_')) {
            --index;
        }
        ++index;
    } else {
        while (((int) index) > 0 && (!isalnum(buffer->text.view.ptr[index]) && buffer->text.view.ptr[index] != '_')) {
            --index;
        }
        ++index;
    }
    return index;
}

size_t buffer_word_boundary_right(Buffer *buffer, size_t index)
{
    size_t max_index = buffer->text.view.length;
    if (isalnum(buffer->text.view.ptr[index]) || buffer->text.view.ptr[index] == '_') {
        while (index < max_index && (isalnum(buffer->text.view.ptr[index]) || buffer->text.view.ptr[index] == '_')) {
            ++index;
        }
    } else {
        while (index < max_index && (!isalnum(buffer->text.view.ptr[index]) && buffer->text.view.ptr[index] != '_')) {
            ++index;
        }
    }
    return index;
}

void buffer_add_listener(Buffer *buffer, BufferEventListener listener)
{
    BufferEventListenerList *list_entry = MALLOC(BufferEventListenerList);
    list_entry->listener = listener;
    list_entry->next = buffer->listeners;
    buffer->listeners = list_entry;
}

StringView buffer_uri(Buffer *buffer)
{
    if (sv_empty(buffer->uri) && sv_not_empty(buffer->name)) {
        buffer->uri = sv_printf("file://%.*s/%.*s", SV_ARG(eddy.project_dir), SV_ARG(buffer->name));
    }
    return buffer->uri;
}

void lsp_semantic_tokens(Buffer *buffer)
{
    lsp_initialize();

    if (sv_empty(buffer->name)) {
        return;
    }
    SemanticTokensParams semantic_tokens_params = { 0 };
    semantic_tokens_params.textDocument = (TextDocumentIdentifier) {
        .uri = buffer_uri(buffer),
    };
    OptionalJSONValue semantic_tokens_params_json = SemanticTokensParams_encode(semantic_tokens_params);
    MUST(Int, lsp_message(buffer, "textDocument/semanticTokens/full", semantic_tokens_params_json));
}

void buffer_semantic_tokens_response(Buffer *buffer, JSONValue resp)
{
    Response response = response_decode(&resp);
    if (!response_success(&response)) {
        return;
    }
    if (!response.result.has_value) {
        trace(LSP, "No response to textDocument/semanticTokens/full");
        return;
    }
    OptionalSemanticTokens result_maybe = SemanticTokens_decode(response.result);
    if (!result_maybe.has_value) {
        trace(LSP, "Couldn't decode response to textDocument/semanticTokens/full");
        return;
    }
    SemanticTokens result = result_maybe.value;
    size_t         lineno = 0;
    Index         *line = buffer->lines.elements + lineno;
    size_t         offset = 0;
    UInt32s        data = result.data;
    size_t         token_ix = 0;
    for (size_t ix = 0; ix < result.data.size; ix += 5) {
//        trace(LSP, "Semantic token[%zu] = (Δline %d, Δcol %d, length %d type %d %d)", ix, data.elements[ix], data.elements[ix + 1], data.elements[ix + 2], data.elements[ix + 3], data.elements[ix + 4]);
        if (data.elements[ix] > 0) {
            lineno += data.elements[ix];
            if (lineno >= buffer->lines.size) {
                // trace(LSP, "Semantic token[%zu] lineno %zu > buffer->lines %zu", ix, lineno, buffer->lines.size);
                break;
            }
            line = buffer->lines.elements + lineno;
            offset = 0;
            token_ix = 0;
        }
        offset += data.elements[ix + 1];
        size_t     length = data.elements[ix + 2];
        StringView text = { line->line.ptr + offset, length };
//        trace(LSP, "Semantic token[%zu]: line: %zu col: %zu length: %zu %.*s", ix, lineno, offset, length, SV_ARG(text));
        OptionalColours colours = theme_semantic_colours(&eddy.theme, data.elements[ix + 3]);
        if (!colours.has_value) {
//            trace(LSP, "SemanticTokenType index %d not mapped", data.elements[ix + 3]);
            continue;
        }
        if (log_category_on(LSP)) {
            StringView s = colour_to_rgb(colours.value.fg);
//            trace(LSP, "Semantic token[%zu] = color '%.*s'", ix, SV_ARG(s));
            sv_free(s);
        }
        for (; token_ix < line->num_tokens; ++token_ix) {
            assert(line->first_token + token_ix < buffer->tokens.size);
            DisplayToken *t = buffer->tokens.elements + line->first_token + token_ix;
            if (t->index == line->index_of + offset && t->length == length) {
                t->color = colour_to_color(colours.value.fg);
                break;
            }
        }
        if (token_ix == line->num_tokens) {
            info("SemanticTokens OUT OF SYNC");
            break;
        }
    }
}

void lsp_on_open(Buffer *buffer)
{
    lsp_initialize();
    if (sv_empty(buffer->name)) {
        return;
    }
    DidOpenTextDocumentParams did_open = { 0 };
    did_open.textDocument = (TextDocumentItem) {
        .uri = buffer_uri(buffer),
        .languageId = sv_from("c"),
        .version = 0,
        .text = buffer->text.view
    };
    OptionalJSONValue did_open_json = DidOpenTextDocumentParams_encode(did_open);
    lsp_notification("textDocument/didOpen", did_open_json);
}

void lsp_did_save(Buffer *buffer)
{
    lsp_initialize();

    if (sv_empty(buffer->name)) {
        return;
    }
    DidSaveTextDocumentParams did_save = { 0 };
    did_save.textDocument = (TextDocumentIdentifier) {
        .uri = buffer_uri(buffer),
    };
    did_save.text = OptionalStringView_create(buffer->text.view);
    OptionalJSONValue did_save_json = DidSaveTextDocumentParams_encode(did_save);
    lsp_notification("textDocument/didSave", did_save_json);
}

void lsp_did_close(Buffer *buffer)
{
    lsp_initialize();

    if (sv_empty(buffer->name)) {
        return;
    }
    DidCloseTextDocumentParams did_close = { 0 };
    did_close.textDocument = (TextDocumentIdentifier) {
        .uri = buffer_uri(buffer),
    };
    OptionalJSONValue did_close_json = DidCloseTextDocumentParams_encode(did_close);
    lsp_notification("textDocument/didClose", did_close_json);
}

void lsp_did_change(Buffer *buffer, IntVector2 start, IntVector2 end, StringView text)
{
    lsp_initialize();

    if (sv_empty(buffer->name)) {
        return;
    }
    DidChangeTextDocumentParams did_change = { 0 };
    did_change.textDocument.uri = buffer_uri(buffer);
    did_change.textDocument.version = buffer->version;
    TextDocumentContentChangeEvent contentChange = { 0 };
    contentChange._0.range.start.line = start.line;
    contentChange._0.range.start.character = start.column;
    contentChange._0.range.end.line = end.line;
    contentChange._0.range.end.character = end.column;
    contentChange._0.text = text;
    da_append_TextDocumentContentChangeEvent(&did_change.contentChanges, contentChange);
    OptionalJSONValue did_change_json = DidChangeTextDocumentParams_encode(did_change);
    lsp_notification("textDocument/didChange", did_change_json);
}
