/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <ctype.h>

#include <eddy.h>
#include <listbox.h>
#include <sys/stat.h>

DA_IMPL(ListBoxEntry)
WIDGET_CLASS_DEF(ListBox, listbox);

void listbox_init(ListBox *listbox)
{
    listbox_sort(listbox);
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
    listbox->lines = (listbox->viewport.height - 19 + eddy.cell.y) / (eddy.cell.y + 2);
    if (listbox->shrink && listbox->no_search && listbox->entries.size < listbox->lines) {
        listbox->lines = listbox->entries.size;
    }
    listbox->viewport.height = 19 + eddy.cell.y + listbox->lines * (eddy.cell.y + 2);
}

void listbox_draw(ListBox *listbox)
{
    widget_draw_rectangle(listbox, 0.0, 0.0, 0.0, 0.0, DARKGRAY);
    widget_draw_outline(listbox, 2, 2, -2.0, -2.0, RAYWHITE);
    widget_render_text(listbox, 8, 8, listbox->prompt, eddy.font, RAYWHITE);
    widget_render_text(listbox, -8, 8, listbox->search.view, eddy.font, RAYWHITE);
    widget_draw_line(listbox, 2, eddy.cell.y + 10, -2, eddy.cell.y + 10, RAYWHITE);
    size_t y = eddy.cell.y + 14;

    ToStringView    to_string_view = listbox->to_string_view;
    size_t          maxlen = listbox->viewport.width / eddy.cell.x;
    ListBoxEntries *entries = (listbox->no_search || sv_empty(listbox->search.view)) ? &listbox->entries : &listbox->matches;
    for (size_t ix = listbox->top_line; ix < entries->size && ix < listbox->top_line + listbox->lines; ++ix) {
        if (ix == listbox->selection) {
            widget_draw_rectangle(listbox, 8, y - 1, -8, eddy.cell.y + 1, palettes[PALETTE_DARK][PI_SELECTION]);
        }
        StringView sv = entries->elements[ix].text;
        if (to_string_view) {
            sv = to_string_view(entries->elements[ix]);
        }
        sv.length = iclamp(sv.length, 0, maxlen);
        widget_render_text(listbox, 10, y, sv, eddy.font, palettes[PALETTE_DARK][PI_DEFAULT]);
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
        if (listbox->dismiss) {
            listbox->dismiss(listbox);
        }
    } else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
        ListBoxEntries *entries = (listbox->no_search || sv_empty(listbox->search.view)) ? &listbox->entries : &listbox->matches;
        if (listbox->selection < entries->size) {
            listbox->status = ModalStatusSubmitted;
            if (listbox->submit) {
                listbox->submit(listbox, entries->elements[listbox->selection]);
            }
        }
    } else if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) {
        if (listbox->selection > 0) {
            --listbox->selection;
            while (listbox->selection < listbox->top_line) {
                --listbox->top_line;
            }
        }
    } else if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) {
        ListBoxEntries *entries = (listbox->no_search || sv_empty(listbox->search.view)) ? &listbox->entries : &listbox->matches;
        if (listbox->selection < entries->size - 1) {
            ++listbox->selection;
            while (listbox->selection > listbox->top_line + listbox->lines - 1) {
                ++listbox->top_line;
            }
        }
    } else if (IsKeyPressed(KEY_PAGE_UP)) {
        if (listbox->selection >= listbox->lines) {
            listbox->selection -= listbox->lines;
            if (listbox->top_line > listbox->lines) {
                listbox->top_line -= listbox->lines;
            }
        }
    } else if (IsKeyPressed(KEY_PAGE_DOWN)) {
        ListBoxEntries *entries = (listbox->no_search || sv_empty(listbox->search.view)) ? &listbox->entries : &listbox->matches;
        if (listbox->selection < entries->size - listbox->lines - 1) {
            listbox->selection += listbox->lines;
            listbox->top_line += listbox->lines;
        }
    }
    if (!listbox->no_search) {
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
    if (listbox->status != ModalStatusActive) {
        --eddy.modals.size;
        da_free_ListBoxEntry(&listbox->entries);
        sv_free(listbox->search.view);
        sv_free(listbox->prompt);
        free(listbox);
    }
}

int listbox_entry_compare(ListBox *, ListBoxEntry *e1, ListBoxEntry *e2)
{
    return sv_cmp(e1->text, e2->text);
}

typedef int (*QSortRCompare)(void *, void const *, void const *);

void listbox_sort(ListBox *listbox)
{
    if (!listbox->no_sort) {
        ListBoxEntryCompare compare = listbox->compare;
        if (!compare) {
            compare = (ListBoxEntryCompare) listbox_entry_compare;
        }
        qsort_r(listbox->entries.elements, listbox->entries.size, sizeof(ListBoxEntry), listbox, (QSortRCompare) compare);
    }
    for (size_t ix = 0; ix < listbox->entries.size; ++ix) {
        listbox->entries.elements[ix].index = ix;
    }
}

void listbox_filter(ListBox *listbox)
{
    if (listbox->no_search) {
        return;
    }
    listbox->matches.size = 0;
    StringView s = listbox->search.view;
    if (sv_empty(s)) {
        for (size_t ix = 0; ix < listbox->entries.size; ++ix) {
            da_append_ListBoxEntry(&listbox->matches, listbox->entries.elements[ix]);
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
            da_append_ListBoxEntry(&listbox->matches, listbox->entries.elements[ix]);
        }
    }
    listbox->selection = 0;
}

void listbox_refresh(ListBox *listbox)
{
    listbox_sort(listbox);
    listbox_filter(listbox);
    if (listbox->shrink && listbox->no_search && listbox->entries.size < listbox->lines) {
        listbox->viewport.height = 19 + eddy.cell.y + listbox->entries.size * (eddy.cell.y + 2);
        listbox->lines = listbox->entries.size;
    }
    listbox->selection = 0;
    listbox->top_line = 0;
    listbox->search.view.length = 0;
    listbox->status = ModalStatusActive;
}

void listbox_show(ListBox *listbox)
{
    listbox_refresh(listbox);
    da_append_Widget(&eddy.modals, (Widget *) listbox);
}

void query_submit(ListBox *query, ListBoxEntry selection)
{
    QueryDef *def = query->memo;
    def->handler(query, (QueryOption) (size_t) selection.payload);
    if (query->status != ModalStatusActive) {
        free(def);
    }
}

void query_dismiss(ListBox *query, ListBoxEntry selection)
{
    QueryDef *def = query->memo;
    def->handler(query, QueryOptionCancel);
    if (query->status != ModalStatusActive) {
        free(def);
    }
}

ListBox *listbox_create_query(StringView query, QueryResult handler, QueryOption options)
{
    ListBox *ret = widget_new(ListBox);
    ret->memo = MALLOC(QueryDef);
    ret->prompt = sv_copy(query);
    ret->submit = (ListBoxSubmit) query_submit;
    ret->dismiss = (ListBoxDismiss) query_dismiss;
    ret->no_sort = true;
    ret->no_search = true;
    ret->shrink = true;
    QueryDef *def = ret->memo;
    def->handler = handler;
    def->options = options;
    if (options | QueryOptionYes) {
        da_append_ListBoxEntry(&ret->entries, (ListBoxEntry) { sv_from("Yes"), (void *) (size_t) QueryOptionYes });
    }
    if (options | QueryOptionNo) {
        da_append_ListBoxEntry(&ret->entries, (ListBoxEntry) { sv_from("No"), (void *) (size_t) QueryOptionCancel });
    }
    if (options | QueryOptionCancel) {
        da_append_ListBoxEntry(&ret->entries, (ListBoxEntry) { sv_from("Cancel"), (void *) (size_t) QueryOptionCancel });
    }
    return ret;
}

void file_selector_populate(ListBox *listbox, StringView directory)
{
    FileSelectorStatus *status = listbox->memo;
    DirListing          dir = status->dir;
    DirOption           dir_option = DirOptionDirectories;
    if (status->options & FSFile) {
        dir_option |= DirOptionFiles;
    }
    if (status->options & FSShowHidden) {
        dir_option |= DirOptionHiddenFiles;
    }
    if (sv_not_empty(dir.directory)) {
        dl_free(dir);
    }
    StringView canonical = fs_canonical(directory);
    status->dir = MUST(DirListing, fs_directory(canonical, dir_option));
    sv_free(canonical);
    listbox->entries.size = 0;
    for (size_t ix = 0; ix < status->dir.entries.size; ++ix) {
        DirEntry *e = da_element_DirEntry(&status->dir.entries, ix);
        da_append_ListBoxEntry(&listbox->entries, (ListBoxEntry) { e->name, e });
    }
    listbox_refresh(listbox);
}

void file_selector_submit(ListBox *listbox, ListBoxEntry selection)
{
    FileSelectorStatus *status = listbox->memo;
    DirListing         *dir = (DirListing *) listbox->memo;
    DirEntry            entry = *(DirEntry *) selection.payload;
    switch (entry.type) {
    case FileTypeDirectory: {
        if (!(status->options & FSDirectory)) {
            StringView filename = sv_from(TextFormat("%.*s/%.*s", SV_ARG(dir->directory), SV_ARG(entry.name)));
            file_selector_populate(listbox, filename);
            break;
        }
    } // Fall through:
    case FileTypeRegularFile: {
        status->handler(listbox, entry);
        if (listbox->status != ModalStatusActive) {
            free(status);
        }
    } break;
    default:
        free(status);
        StringView filename = sv_from(TextFormat("%.*s/%.*s", SV_ARG(dir->directory), SV_ARG(entry.name)));
        StringView canonical = fs_canonical(filename);
        eddy_set_message(&eddy, "Cannot open non-regular file '%.*'", SV_ARG(canonical));
        sv_free(canonical);
    }
}

void file_selector_dismiss(ListBox *listbox)
{
    FileSelectorStatus *status = listbox->memo;
    dl_free(status->dir);
    free(status);
}

StringView file_selector_to_string_view(ListBoxEntry entry)
{
    DirEntry dir_entry = *(DirEntry *) entry.payload;
    if (dir_entry.type & FileTypeDirectory) {
        return sv_printf("> %.*s", SV_ARG(dir_entry.name));
    }
    return sv_printf("  %.*s", SV_ARG(dir_entry.name));
}

int file_selector_compare(ListBox *, ListBoxEntry const* e1, ListBoxEntry const *e2)
{
    DirEntry *d1 = e1->payload;
    DirEntry *d2 = e2->payload;
    if (d1->type & FileTypeDirectory && !(d2->type & FileTypeDirectory)) {
        return -1;
    }
    if (!(d1->type & FileTypeDirectory) && d2->type & FileTypeDirectory) {
        return 1;
    }
    return sv_icmp(d1->name, d2->name);
}

void file_selector_process_input(ListBox *listbox)
{
    FileSelectorStatus *status = listbox->memo;
    DirListing         *dir = (DirListing *) listbox->memo;
    StringView filename = {0};
    if (IsKeyPressed(KEY_LEFT)) {
        filename = sv_from(TextFormat("%.*s/..", SV_ARG(dir->directory)));
    }
    if (IsKeyPressed(KEY_RIGHT)) {
        DirEntry entry = *(DirEntry *) listbox->entries.elements[listbox->selection].payload;
        if (entry.type == FileTypeDirectory) {
            filename = sv_from(TextFormat("%.*s/%.*s", SV_ARG(dir->directory), SV_ARG(entry.name)));
        }
    }
    if (sv_not_empty(filename)) {
        file_selector_populate(listbox, filename);
        return;
    }
    listbox_process_input(listbox);
}

ListBox *file_selector_create(StringView prompt, FileSelectorResult handler, FileSelectorOption options)
{
    ListBox            *ret = widget_new(ListBox);
    FileSelectorStatus *status = MALLOC(FileSelectorStatus);
    status->options = options;
    status->handler = handler;
    ret->memo = status;
    ret->handlers.process_input = (WidgetProcessInput) file_selector_process_input;
    ret->submit = file_selector_submit;
    ret->dismiss = file_selector_dismiss;
    ret->to_string_view = file_selector_to_string_view;
    ret->compare = (ListBoxEntryCompare) file_selector_compare;
    ret->prompt = sv_from("Select file");
    StringView canonical = fs_canonical(sv_from("."));
    file_selector_populate(ret, canonical);
    sv_free(canonical);
    return ret;
}
