/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <math.h>
#include <stdarg.h>

#include <app/eddy.h>
#include <app/minibuffer.h>

WIDGET_CLASS_DEF(MiniBuffer, minibuffer);
WIDGET_CLASS_DEF(MiniBufferQuery, mb_query);

void mb_query_process_input(MiniBufferQuery *mbq)
{
    if (IsKeyPressed(KEY_ESCAPE)) {
        --eddy.modals.size;
        mbq->fnc = NULL;
        mbq->cursor = 0;
        mbq->text.length = 0;
        mbq->prompt = sv_null();
    } else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
        MiniBufferChain chain = mbq->fnc(mbq->target, mbq->text.view);
        mbq->cursor = 0;
        mbq->text.length = 0;
        if (chain.fnc == NULL) {
            --eddy.modals.size;
            mbq->fnc = NULL;
            mbq->prompt = sv_null();
        } else {
            mbq->fnc = (MiniBufferQueryFunction) chain.fnc;
            mbq->prompt = chain.prompt;
        }
    } else if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
        if (mbq->cursor > 0) {
            sb_remove(&mbq->text, mbq->cursor - 1, 1);
            --mbq->cursor;
        }
    } else if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) {
        if (mbq->cursor > 0) {
            --mbq->cursor;
        }
    } else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) {
        if (mbq->text.length > 0 && mbq->cursor < mbq->text.length - 1) {
            ++mbq->cursor;
        }
    } else if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) {
        if (mbq->cursor > 0) {
            --mbq->cursor;
        }
    } else if (IsKeyPressed(KEY_HOME) || IsKeyPressedRepeat(KEY_HOME)) {
        mbq->cursor = 0;
    } else if (IsKeyPressed(KEY_END) || IsKeyPressedRepeat(KEY_END)) {
        mbq->cursor = mbq->text.length - 1;
    }
}

bool mb_query_character(MiniBufferQuery *mbq, int ch)
{
    sb_insert_chars(&mbq->text, (char *) &ch, 1, mbq->cursor);
    ++mbq->cursor;
    return true;
}

void mb_query_init(MiniBufferQuery *mbq)
{
    mbq->policy = SP_CHARACTERS;
    mbq->policy_size = 1.0f;
    mbq->padding = DEFAULT_PADDING;
    mbq->handlers.character = (WidgetHandleCharacter) mb_query_character;
}

void mb_query_resize(MiniBufferQuery *mbq)
{
}

void mb_query_draw(MiniBufferQuery *mbq)
{
    if (!sv_empty(mbq->prompt)) {
        widget_render_text(mbq, 0, 0, mbq->prompt, eddy.font, palettes[PALETTE_DARK][PI_DEFAULT]);
        widget_render_text(mbq, mbq->prompt.length * eddy.cell.x, 0, SV(": ", 2), eddy.font, palettes[PALETTE_DARK][PI_DEFAULT]);
    }
    if (!sv_empty(mbq->text.view)) {
        widget_render_text(mbq, (mbq->prompt.length + 2) * eddy.cell.x, 0, mbq->text.view, eddy.font, palettes[PALETTE_DARK][PI_DEFAULT]);
    }
    double t = GetTime();
    if ((t - floor(t)) < 0.5) {
        int x = mbq->prompt.length + 2 + mbq->cursor;
        widget_draw_rectangle(mbq, x * eddy.cell.x, 0, 2, eddy.cell.y + 5, palettes[PALETTE_DARK][PI_CURSOR]);
    }
}

void minibuffer_init(MiniBuffer *minibuffer)
{
    minibuffer->policy = SP_CHARACTERS;
    minibuffer->policy_size = 1.0f;
    minibuffer->padding = DEFAULT_PADDING;
    minibuffer->message = sv_null();
    in_place_widget(MiniBufferQuery, &minibuffer->current_query, NULL);
    minibuffer->current_query.viewport = minibuffer->viewport;
    minibuffer->current_query.padding = minibuffer->padding;
}

void minibuffer_resize(MiniBuffer *minibuffer)
{
    minibuffer->current_query.viewport = minibuffer->viewport;
    minibuffer->current_query.padding = minibuffer->padding;
}

void minibuffer_draw(MiniBuffer *minibuffer)
{
    if (minibuffer->current_query.fnc != NULL) {
        return;
    }
    widget_draw_rectangle(minibuffer, 0, 0, minibuffer->viewport.width, minibuffer->viewport.height, palettes[PALETTE_DARK][PI_BACKGROUND]);
    if (!sv_empty(minibuffer->message)) {
        widget_render_text(minibuffer, 0, 0, minibuffer->message, eddy.font, palettes[PALETTE_DARK][PI_DEFAULT]);
    }
}

void minibuffer_process_input(MiniBuffer *minibuffer)
{
    if (sv_not_empty(minibuffer->message) && eddy.time - minibuffer->time > 2.0) {
        sv_free(minibuffer->message);
        minibuffer->message = sv_null();
    }
}

void minibuffer_set_vmessage_internal(MiniBuffer *minibuffer, char const *fmt, va_list args)
{
    if (!sv_empty(minibuffer->message)) {
        sv_free(minibuffer->message);
    }
    minibuffer->message = sv_vprintf(fmt, args);
    minibuffer->time = eddy.time;
}

void minibuffer_set_message_internal(MiniBuffer *minibuffer, char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    minibuffer_set_vmessage_internal(minibuffer, fmt, args);
    va_end(args);
}

void minibuffer_clear(MiniBuffer *minibuffer)
{
    if (sv_not_empty(minibuffer->message)) {
        sv_free(minibuffer->message);
        minibuffer->message = sv_null();
    }
}

void minibuffer_set_message(char const *fmt, ...)
{
    MiniBuffer *minibuffer = (MiniBuffer *) layout_find_by_classname((Layout *) &eddy, SV("MiniBuffer", 10));
    assert(minibuffer);
    va_list args;
    va_start(args, fmt);
    minibuffer_set_vmessage_internal(minibuffer, fmt, args);
    va_end(args);
}

void minibuffer_set_vmessage(char const *fmt, va_list args)
{
    MiniBuffer *minibuffer = (MiniBuffer *) layout_find_by_classname((Layout *) &eddy, sv_from("MiniBuffer"));
    assert(minibuffer);
    minibuffer_set_vmessage_internal(minibuffer, fmt, args);
    va_end(args);
}

void minibuffer_set_message_sv(StringView message)
{
    MiniBuffer *minibuffer = (MiniBuffer *) layout_find_by_classname((Layout *) &eddy, SV("MiniBuffer", 10));
    minibuffer->message = sv_copy(message);
    minibuffer->time = eddy.time;
}

void minibuffer_clear_message()
{
    MiniBuffer *minibuffer = (MiniBuffer *) layout_find_by_classname((Layout *) &eddy, SV("MiniBuffer", 10));
    assert(minibuffer);
    minibuffer_clear(minibuffer);
}

void minibuffer_query(void *w, StringView prompt, MiniBufferQueryFunction fnc)
{
    MiniBuffer *minibuffer = (MiniBuffer *) layout_find_by_classname((Layout *) &eddy, SV("MiniBuffer", 10));
    assert(minibuffer != NULL);
    assert(fnc != NULL);
    if (minibuffer->current_query.fnc != NULL) {
        minibuffer_set_message_internal(minibuffer, "Minibuffer already active");
        return;
    }
    minibuffer_clear(minibuffer);
    minibuffer->current_query.fnc = fnc;
    minibuffer->current_query.target = w;
    minibuffer->current_query.prompt = prompt;
    minibuffer->current_query.text.length = 0;
    da_append_Widget(&eddy.modals, (Widget *) &minibuffer->current_query);
}
