/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __APP_LISTBOX_H__
#define __APP_LISTBOX_H__

#include <widget.h>

typedef struct {
    StringView text;
    void      *payload;
} ListBoxEntry;

DA_WITH_NAME(ListBoxEntry, ListBoxEntries);

typedef struct {
    _W;
    StringView     prompt;
    ListBoxEntries entries;
    StringList     matches;
    StringBuilder  search;
    int            lines;
    int            top_line;
    int            selection;
    int            selected_entry;
    ModalStatus    status;
} ListBox;

WIDGET_CLASS(ListBox, listbox)

void listbox_filter(ListBox *listbox);

#endif /* __APP_LISTBOX_H__ */
