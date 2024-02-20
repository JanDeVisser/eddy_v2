/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <ctype.h>

#include <eddy.h>
#include <listbox.h>

DA_IMPL(ListBoxEntry)
WIDGET_CLASS_DEF(ListBox, listbox);

void listbox_init(ListBox *listbox)
{
    listbox_filter(listbox);
    listbox->background = DARKGRAY;
    listbox_resize(listbox);
}

void listbox_resize(ListBox *listbox)
{
    listbox->viewport.x = eddy.viewport.width / 4;
    listbox->viewport.y = eddy.viewport.height / 4;
    listbox->viewport.width = eddy.viewport.width / 2;
    listbox->viewport.height = eddy.viewport.height / 2;
    listbox->lines = (listbox->viewport.height - 10) / (eddy.cell.y + 2);
}

void listbox_draw(ListBox *listbox)
{
    widget_draw_rectangle(listbox, 0.0, 0.0, 0.0, 0.0, DARKGRAY);
    widget_draw_outline(listbox, 2, 2, -4.0, -4.0, RAYWHITE);
    widget_render_text(listbox, 8, 8, listbox->prompt, eddy.font, RAYWHITE);
    widget_render_text(listbox, -8, 8, listbox->search.view, eddy.font, RAYWHITE);
    size_t y = eddy.cell.y + 10;
    for (size_t ix = listbox->top_line; ix < listbox->matches.size && ix < listbox->top_line + listbox->lines - 1; ++ix) {
        if (ix == listbox->selection) {
            widget_draw_rectangle(listbox, 4, y - 1, -4, eddy.cell.y + 1, palettes[PALETTE_DARK][PI_CURRENT_LINE_FILL]);
            widget_draw_outline(listbox, 4, y - 1, -4, eddy.cell.y + 1, palettes[PALETTE_DARK][PI_CURRENT_LINE_EDGE]);
        }
        widget_render_text(listbox, 10, y, listbox->matches.strings[ix], eddy.font, palettes[PALETTE_DARK][PI_DEFAULT]);
        y += eddy.cell.y + 2;
    }
}

void listbox_process_input(ListBox *listbox)
{
    if (listbox->status == ModalStatusDormant) {
        return;
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        listbox->status = ModalStatusDismissed;
        return;
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
        StringView selected_text = listbox->matches.strings[listbox->selection];
        listbox->selected_entry = -1;
        for (size_t ix = 0; ix < listbox->entries.size; ++ix) {
            StringView text = listbox->entries.elements[ix].text;
            if (sv_eq(text, selected_text)) {
                listbox->selected_entry = ix;
                break;
            }
        }
        assert(listbox->selected_entry >= 0);
        listbox->status = ModalStatusSubmitted;
        return;
    }
    if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) {
        if (listbox->selection > 0) {
            --listbox->selection;
            while (listbox->selection < listbox->top_line) {
                --listbox->top_line;
            }
        }
        return;
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) {
        if (listbox->selection < listbox->matches.size - 1) {
            ++listbox->selection;
            while (listbox->selection > listbox->top_line + listbox->lines - 2) {
                ++listbox->top_line;
            }
        }
        return;
    }
    if (IsKeyPressed(KEY_PAGE_UP)) {
        if (listbox->selection >= listbox->lines) {
            listbox->selection -= listbox->lines;
            if (listbox->top_line > listbox->lines) {
                listbox->top_line -= listbox->lines;
            }
        }
        return;
    }
    if (IsKeyPressed(KEY_PAGE_DOWN)) {
        if (listbox->selection < listbox->matches.size - listbox->lines - 1) {
            listbox->selection += listbox->lines;
            listbox->top_line += listbox->lines;
        }
        return;
    }
    int len = listbox->search.view.length;
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
        if (!sv_empty(listbox->search.view)) {
            sb_remove(&listbox->search, len - 1, 1);
        }
    }
    for (int ch = GetCharPressed(); ch != 0; ch = GetCharPressed()) {
        sb_append_char(&listbox->search, ch);
    }
    if (listbox->search.view.length != len) {
        listbox_filter(listbox);
    }
}

void listbox_filter(ListBox *listbox)
{
    listbox->matches.size = 0;
    StringView s = listbox->search.view;
    if (sv_empty(s)) {
        for (size_t ix = 0; ix < listbox->entries.size; ++ix) {
            sl_push(&listbox->matches, listbox->entries.elements[ix].text);
        }
        return;
    }
    for (size_t ix = 0; ix < listbox->entries.size; ++ix) {
        StringView text = listbox->entries.elements[ix].text;
        size_t     tix = 0;
        while (tix < text.length && tix < s.length && toupper(s.ptr[tix]) == toupper(text.ptr[tix])) {
            ++tix;
        }
        if (tix == s.length) {
            sl_push(&listbox->matches, text);
        }
    }
}
