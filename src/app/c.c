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
    eddy_set_message(&eddy, ctx->called_as);

    if (sv_not_empty(buffer->name)) {
        StringView output = MUST(
            StringView,
            execute_pipe(
                buffer->text.view,
                sv_from("clang-format"),
                "--output-replacements-xml",
                TextFormat("--assume-filename=%.*s", SV_ARG(buffer->name)),
                TextFormat("--cursor=%d", view->cursor)));

        trace(CAT_LSP, "clang-format: %.*s\n", SV_ARG(output));
        XMLNode         doc = MUST(XMLNode, xml_deserialize(output));
        StringView doc_sv = xml_serialize(doc);
        trace(CAT_LSP, "doc serialized %.*s\n", SV_ARG(doc_sv));
        OptionalXMLNode replacements_maybe = xml_first_child_by_tag(doc, sv_from("replacements"));
        if (!replacements_maybe.has_value) {
            return;
        }
        XMLNodes replacements = xml_children_by_tag(replacements_maybe.value, sv_from("replacement"));
        trace(CAT_LSP, "clang-format returned %zu replacements", replacements.size);
        for (size_t ix = 0; ix < replacements.size; ++ix) {
            XMLNode            repl = replacements.elements[ix];
            StringView         replacement_text = sv_null();
            OptionalStringView repl_text_maybe = xml_text_of(repl);
            if (repl_text_maybe.has_value) {
                replacement_text = repl_text_maybe.value;
            }
            trace(CAT_LSP, "[%zu] replacement '%.*s'", ix, SV_ARG(replacement_text));
            EditType           edit_type = ETReplace;
            XMLNode            attr = MUST_OPTIONAL(XMLNode, xml_attribute_by_tag(repl, sv_from("length")));
            StringView         attr_text = MUST_OPTIONAL(StringView, xml_text_of(attr));
            trace(CAT_LSP, "[%zu] length '%.*s'", ix, SV_ARG(attr_text));
            IntegerParseResult parse_result = sv_parse_u32(attr_text);
            assert(parse_result.success);
            uint32_t length = parse_result.integer.u32;
            attr = MUST_OPTIONAL(XMLNode, xml_attribute_by_tag(repl, sv_from("offset")));
            attr_text = MUST_OPTIONAL(StringView, xml_text_of(attr));
            trace(CAT_LSP, "[%zu] offset '%.*s'", ix, SV_ARG(attr_text));
            parse_result = sv_parse_u32(attr_text);
            assert(parse_result.success);
            uint32_t offset = parse_result.integer.u32;
            if (sv_empty(replacement_text)) {
                buffer_delete(buffer, offset, length);
            } else if (length == 0) {
                buffer_insert(buffer, replacement_text, offset);
            } else {
                buffer_replace(buffer, offset, length, replacement_text);
            }
        }
        OptionalXMLNode cursor_maybe = xml_first_child_by_tag(replacements_maybe.value, sv_from("cursor"));
        if (cursor_maybe.has_value) {
            StringView position = MUST_OPTIONAL(StringView, xml_text_of(cursor_maybe.value));
            IntegerParseResult parse_result = sv_parse_u32(position);
            assert(parse_result.success);
            uint32_t pos = parse_result.integer.u32;
            view->new_cursor = pos;
            view->cursor_col = -1;
            view->selection = -1;
        }
        xml_free(doc);
        eddy_set_message(&eddy, sv_from("Formatted"));
        return;
    }
    eddy_set_message(&eddy, sv_from("Cannot format untitled buffer"));
}

void c_mode_on_draw(CMode *mode)
{
    BufferView *view = (BufferView *) mode->parent;
    lsp_semantic_tokens(view->buffer_num);
}

void c_mode_init(CMode *mode)
{
    widget_add_command(mode, sv_from("c-format-source"), c_mode_cmd_format_source,
        (KeyCombo) { KEY_L, KMOD_SHIFT | KMOD_CONTROL });
    mode->handlers.on_draw = (WidgetOnDraw) c_mode_on_draw;
    BufferView *view = (BufferView *) mode->parent;
    lsp_on_open(view->buffer_num);
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
    char const *buffer = lexer_source(lexer).ptr;
    size_t      ix = 0;
    size_t      state = (size_t) lexer->language_data;
    switch (state) {
    case CDirectiveStateInit: {
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
