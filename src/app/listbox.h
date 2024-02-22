/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __APP_LISTBOX_H__
#define __APP_LISTBOX_H__

#include <fs.h>
#include <widget.h>

typedef enum : uint8_t {
    FSFile = 0x01,
    FSDirectory = 0x02,
    FSShowHidden = 0x04,
} FileSelectorOption;

typedef enum : uint8_t {
    QueryOptionYes = 0x01,
    QueryOptionNo = 0x02,
    QueryOptionYesNo = 0x03,
    QueryOptionCancel = 0x04,
    QueryOptionYesNoCancel = 0x07,
} QueryOption;

typedef struct {
    StringView text;
    void      *payload;
    size_t     index;
} ListBoxEntry;

typedef struct listbox ListBox;
typedef void           (*FileSelectorResult)(ListBox *listbox, DirEntry entry);
typedef void           (*QueryResult)(ListBox *listbox, QueryOption selected);

typedef struct {
    QueryResult handler;
    QueryOption options;
} QueryDef;

typedef struct {
    DirListing         dir;
    FileSelectorOption options;
    FileSelectorResult handler;
} FileSelectorStatus;

DA_WITH_NAME(ListBoxEntry, ListBoxEntries);

typedef int        (*ListBoxEntryCompare)(ListBox *, ListBoxEntry const *, ListBoxEntry const *);
typedef StringView (*ToStringView)(ListBoxEntry);
typedef void       (*ListBoxSubmit)(ListBox *listbox, ListBoxEntry selection);
typedef void       (*ListBoxDismiss)(ListBox *listbox);

typedef struct listbox {
    _W;
    StringView          prompt;
    ListBoxEntries      entries;
    ListBoxEntries      matches;
    StringBuilder       search;
    int                 lines;
    int                 top_line;
    int                 selection;
    bool                no_sort;
    bool                no_search;
    bool                shrink;
    ModalStatus         status;
    ListBoxEntryCompare compare;
    ToStringView        to_string_view;
    ListBoxSubmit       submit;
    ListBoxDismiss      dismiss;
} ListBox;

WIDGET_CLASS(ListBox, listbox)

extern void     listbox_sort(ListBox *listbox);
extern void     listbox_filter(ListBox *listbox);
extern void     listbox_refresh(ListBox *listbox);
extern void     listbox_show(ListBox *listbox);
extern ListBox *listbox_create_query(StringView query, QueryResult handler, QueryOption options);
extern ListBox *file_selector_create(StringView prompt, FileSelectorResult handler, FileSelectorOption options);

#endif /* __APP_LISTBOX_H__ */
