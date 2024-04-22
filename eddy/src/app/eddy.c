/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <math.h>
#include <pwd.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <app/c.h>
#include <app/cmake.h>
#include <app/eddy.h>
#include <app/listbox.h>
#include <app/minibuffer.h>
#include <base/fs.h>
#include <base/io.h>
#include <base/json.h>
#include <base/options.h>
#include <lsp/lsp.h>
#include <lsp/schema/Diagnostic.h>
#include <lsp/schema/PublishDiagnosticsParams.h>

#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 768

void eddy_load_theme(Eddy *eddy, StringView theme_name);

AppState   app_state = { 0 };
Eddy       eddy = { 0 };
FT_Library ft_library = { 0 };

LAYOUT_CLASS_DEF(StatusBar, sb);

void app_state_read(AppState *state)
{
    struct passwd *pw = getpwuid(getuid());
    struct stat    sb;
    char const    *eddy_fname = TextFormat("%s/.eddy", pw->pw_dir);
    if (stat(eddy_fname, &sb) != 0) {
        mkdir(eddy_fname, 0700);
    }

    char const *state_fname = TextFormat("%s/.eddy/state", pw->pw_dir);
    if (stat(state_fname, &sb) == 0) {
        int state_fd = open(state_fname, O_RDONLY);
        read(state_fd, state, sizeof(AppState));
        close(state_fd);
    } else {
        app_state_write(state);
    }
}

void app_state_write(AppState *state)
{
    struct passwd *pw = getpwuid(getuid());
    char const    *state_fname = TextFormat("%s/.eddy/state", pw->pw_dir);
    int            state_fd = open(state_fname, O_RDWR | O_CREAT | O_TRUNC, 0600);
    write(state_fd, state, sizeof(AppState));
    close(state_fd);
}

void sb_file_name_resize(Label *label)
{
    label->background = colour_to_color(eddy.theme.selection.bg);
    label->color = colour_to_color(eddy.theme.selection.fg);
}

void sb_file_name_draw(Label *label)
{
    if (!label->parent->memo) {
        label->parent->memo = layout_find_by_draw_function((Layout *) label->parent->parent, (WidgetDraw) editor_draw);
    }
    Editor *editor = (Editor *) label->parent->memo;
    assert(editor);
    BufferView *view = editor->buffers.elements + eddy.editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    if (sv_empty(buffer->name)) {
        label->text = sv_from(TextFormat("untitled-%d%c", view->buffer_num, buffer->saved_version != buffer->undo_stack.size ? '*' : ' '));
    } else {
        label->text = sv_from(TextFormat("%.*s%c", SV_ARG(buffer->name), buffer->saved_version != buffer->undo_stack.size ? '*' : ' '));
    }
    label_draw(label);
}

void sb_cursor_resize(Label *label)
{
    label->background = colour_to_color(eddy.theme.selection.bg);
    label->color = colour_to_color(eddy.theme.selection.fg);
}

void sb_cursor_draw(Label *label)
{
    if (!label->parent->memo) {
        label->parent->memo = layout_find_by_draw_function((Layout *) label->parent->parent, (WidgetDraw) editor_draw);
    }
    Editor *editor = (Editor *) label->parent->memo;
    assert(editor);
    BufferView *view = editor->buffers.elements + eddy.editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    float       where = ((float) view->cursor_pos.line / (float) buffer->lines.size) * 100.0f;
    label->text = sv_from(TextFormat("ln %d (%d%%) col %d", view->cursor_pos.line + 1, (int) roundf(where), view->cursor_pos.column + 1));
    label_draw(label);
}

void sb_fps_draw(Label *label)
{
    int fps = GetFPS();
    label->text = sv_from(TextFormat("%d", fps));
    if (fps > 55) {
        label->color = GREEN;
    } else if (fps > 35) {
        label->color = ORANGE;
    } else {
        label->color = RED;
    }
    label_draw(label);
}

void sb_on_draw(StatusBar *sb)
{
    widget_draw_rectangle(sb, 0, 0, 0, 0, colour_to_color(eddy.theme.selection.bg));
}

void sb_init(StatusBar *status_bar)
{
    status_bar->orientation = CO_HORIZONTAL;
    status_bar->policy = SP_CHARACTERS;
    status_bar->policy_size = 1.0f;
    status_bar->handlers.on_draw = (WidgetOnDraw) sb_on_draw;
    layout_add_widget((Layout *) status_bar, (Widget *) widget_new_with_policy(Spacer, SP_CHARACTERS, 1));
    Label *file_name = (Label *) widget_new(Label);
    file_name->policy_size = 40;
    file_name->color = colour_to_color(eddy.theme.selection.fg);
    file_name->handlers.draw = (WidgetDraw) sb_file_name_draw;
    file_name->handlers.resize = (WidgetResize) sb_file_name_resize;
    layout_add_widget((Layout *) status_bar, (Widget *) file_name);
    layout_add_widget((Layout *) status_bar, (Widget *) widget_new(Spacer));
    Label *cursor = widget_new(Label);
    cursor->policy_size = 21;
    cursor->color = colour_to_color(eddy.theme.selection.fg);
    cursor->handlers.draw = (WidgetDraw) sb_cursor_draw;
    cursor->handlers.resize = (WidgetResize) sb_cursor_resize;
    layout_add_widget((Layout *) status_bar, (Widget *) cursor);
    Label *last_key = widget_new(Label);
    Label *fps = widget_new(Label);
    fps->policy_size = 4;
    fps->color = colour_to_color(eddy.theme.selection.fg);
    fps->handlers.draw = (WidgetDraw) sb_fps_draw;
    layout_add_widget((Layout *) status_bar, (Widget *) fps);
}

APP_CLASS_DEF(Eddy, eddy);

void eddy_quit(Eddy *e)
{
    JSONValue state = json_object();
    JSONValue files = json_array();
    for (size_t ix = 0; ix < e->buffers.size; ++ix) {
        if (sv_not_empty(e->buffers.elements[ix].name)) {
            StringView rel = fs_relative(e->buffers.elements[ix].name, e->project_dir);
            json_append(&files, json_string(rel));
        }
    }
    json_set(&state, "files", files);
    MUST(Size, write_file_by_name(sv_from(".eddy/state"), json_encode(state)));
    e->quit = true;
}

void eddy_are_you_sure_handler(ListBox *, QueryOption selection)
{
    if (selection == QueryOptionYes) {
        eddy_quit(&eddy);
    }
}

void eddy_cmd_force_quit(Eddy *e, JSONValue)
{
    eddy_quit(e);
}

void eddy_cmd_quit(Eddy *e, JSONValue)
{
    if (e->modals.size > 0) {
        // User probably clicked the close window button twice
        return;
    }
    bool has_modified_buffers = false;
    for (size_t ix = 0; ix < e->buffers.size; ++ix) {
        Buffer *buffer = da_element_Buffer(&e->buffers, ix);
        if (buffer->saved_version < buffer->version) {
            has_modified_buffers = true;
            break;
        }
    }
    int        selection = 0;
    StringView prompt = SV("Are you sure you want to quit?", 30);
    if (has_modified_buffers) {
        selection = 1;
        prompt = SV("There are modified files. Are you sure you want to quit?", 56);
    }
    ListBox *are_you_sure = listbox_create_query(prompt, eddy_are_you_sure_handler, QueryOptionYesNo);
    listbox_show(are_you_sure);
    are_you_sure->selection = selection;
}

void run_command_submit(ListBox *, ListBoxEntry selection)
{
    WidgetCommand *cmd = (WidgetCommand *) selection.payload;
    app_submit((App *) &eddy, cmd->owner, cmd->command, json_null());
    eddy_set_message(&eddy, "Selected command '%.*s'", SV_ARG(cmd->command));
}

void eddy_cmd_run_command(Eddy *e, JSONValue unused)
{
    ListBox *listbox = widget_new(ListBox);
    listbox->prompt = sv_from("Select command");
    listbox->submit = run_command_submit;
    for (Widget *w = e->focus; w; w = w->parent) {
        for (size_t cix = 0; cix < w->commands.size; ++cix) {
            WidgetCommand *command = w->commands.elements + cix;
            da_append_ListBoxEntry(&listbox->entries, (ListBoxEntry) { .text = command->command, .payload = command });
        }
        if (w->delegate) {
            for (size_t cix = 0; cix < w->delegate->commands.size; ++cix) {
                WidgetCommand *command = w->delegate->commands.elements + cix;
                da_append_ListBoxEntry(&listbox->entries, (ListBoxEntry) { .text = command->command, .payload = command });
            }
        }
    }
    listbox_show(listbox);
}

void eddy_open_fs_handler(ListBox *listbox, DirEntry entry)
{
    FileSelectorStatus *status = listbox->memo;
    DirListing         *dir = (DirListing *) listbox->memo;
    StringView          filename = sv_printf("%.*s/%.*s", SV_ARG(dir->directory), SV_ARG(entry.name));
    StringView          canonical = fs_canonical(filename);
    sv_free(filename);
    ErrorOrBuffer buffer_maybe = eddy_open_buffer(&eddy, canonical);
    if (ErrorOrBuffer_is_error(buffer_maybe)) {
        eddy_set_message(&eddy, "Could not open '%.*': %s", SV_ARG(canonical), Error_to_string(buffer_maybe.error));
    } else {
        editor_select_buffer(eddy.editor, buffer_maybe.value->buffer_ix);
    }
    sv_free(canonical);
}

void eddy_cmd_open_file(Eddy *e, JSONValue)
{
    ListBox *listbox = file_selector_create(SV("Select file", 11), eddy_open_fs_handler, FSFile);
    listbox_show(listbox);
}

void free_search_entry(ListBox *, ListBoxEntry entry)
{
    sv_free(entry.string);
    sv_free(entry.text);
}

void file_search_submit(ListBox *, ListBoxEntry selection)
{
    StringView    filename = selection.string;
    StringView    canonical = fs_canonical(filename);
    ErrorOrBuffer buffer_maybe = eddy_open_buffer(&eddy, canonical);
    if (ErrorOrBuffer_is_error(buffer_maybe)) {
        eddy_set_message(&eddy, "Could not open '%.*': %s", SV_ARG(canonical), Error_to_string(buffer_maybe.error));
    } else {
        editor_select_buffer(eddy.editor, buffer_maybe.value->buffer_ix);
    }
    sv_free(canonical);
}

ErrorOrInt fill_search_listbox(ListBox *listbox, StringView dir)
{
    DirListing listing = TRY_TO(DirListing, Int, fs_directory(dir, DirOptionFiles | DirOptionDirectories));
    for (size_t ix = 0; ix < listing.entries.size; ++ix) {
        DirEntry *entry = da_element_DirEntry(&listing.entries, ix);
        switch (entry->type) {
        case FileTypeRegularFile: {
            StringView label = sv_printf("%.*s (%.*s)", SV_ARG(entry->name), SV_ARG(dir));
            StringView fullpath = sv_printf("%.*s/%.*s", SV_ARG(dir), SV_ARG(entry->name));
            da_append_ListBoxEntry(&listbox->entries, (ListBoxEntry) { .text = label, .string = fullpath });
        } break;
        case FileTypeDirectory: {
            if (entry->name.length && (entry->name.ptr[0] == '.')) {
                continue;
            }
            StringView subdir = sv_printf("%.*s/%.*s", SV_ARG(dir), SV_ARG(entry->name));
            ErrorOrInt error_maybe = fill_search_listbox(listbox, subdir);
            sv_free(subdir);
            if (ErrorOrInt_is_error(error_maybe)) {
                return error_maybe;
            }
        } break;
        default:
            UNREACHABLE();
        }
    }
    dl_free(listing);
    RETURN(Int, 0);
}

void eddy_cmd_search_file(Eddy *e, JSONValue unused)
{
    ListBox *listbox = widget_new(ListBox);
    listbox->submit = file_search_submit;
    listbox->free_entry = free_search_entry;
    for (size_t ix = 0; ix < e->source_dirs.size; ++ix) {
        ErrorOrInt error_maybe = fill_search_listbox(listbox, e->source_dirs.strings[ix]);
        if (ErrorOrInt_is_error(error_maybe)) {
            eddy_set_message(e, "Could not list source directory '%.*': %s", SV_ARG(e->source_dirs.strings[ix]), Error_to_string(error_maybe.error));
            listbox_free(listbox);
            return;
        }
    }
    listbox_show(listbox);
}

void eddy_cmd_set_message(Eddy *, JSONValue message)
{
    assert(message.type == JSON_TYPE_STRING);
    minibuffer_set_message_sv(message.string);
}

void eddy_cmd_display_messagebox(Eddy *, JSONValue message)
{
    assert(message.type == JSON_TYPE_STRING);
    ListBox *box = messagebox_create(message.string);
    listbox_show(box);
}

StringList eddy_get_font_dirs(Eddy *e)
{
    static StringList s_font_dirs = { 0 };
    if (s_font_dirs.size == 0) {
        JSONValue appearance = json_get_default(&e->settings, "appearance", json_object());
        JSONValue directories = json_get_default(&appearance, "font_directories", json_array());
        if (directories.type == JSON_TYPE_STRING) {
            JSONValue dirs = json_array();
            json_append(&dirs, json_copy(directories));
            directories = dirs;
        }
        assert(directories.type == JSON_TYPE_ARRAY);
        StringView     packaged_font_dir = fs_canonical(sv_from(EDDY_DATADIR "/fonts"));
        struct passwd *pw = getpwuid(getuid());
        StringBuilder  d = { 0 };
        for (size_t ix = 0; ix < json_len(&directories); ++ix) {
            JSONValue dir = MUST_OPTIONAL(JSONValue, json_at(&directories, ix));
            assert(dir.type == JSON_TYPE_STRING);
            d = (StringBuilder) { 0 };
            sb_append_sv(&d, dir.string);
            sb_replace_all(&d, SV("${HOME}", 7), sv_from(pw->pw_dir));
            sb_replace_all(&d, SV("${EDDY_DATADIR}", 15), sv_from(EDDY_DATADIR));
            StringView dirname = fs_canonical(d.view);
            if (sv_eq(dirname, packaged_font_dir)) {
                sv_free(packaged_font_dir);
                packaged_font_dir = (StringView) { 0 };
            }
            sl_push(&s_font_dirs, d.view);
        }
        if (packaged_font_dir.length > 0) {
            sl_push(&s_font_dirs, packaged_font_dir);
        }
    }
    return s_font_dirs;
}

void eddy_inc_font_size(Eddy *e, int increment)
{
    app_set_font((App *) e, e->font_path, e->font_size + increment);
    eddy_set_message(e, "Font size %d", e->font_size);
}

void eddy_cmd_shrink_font(Eddy *e, JSONValue)
{
    eddy_inc_font_size(e, -1);
}

void eddy_cmd_enlarge_font(Eddy *e, JSONValue)
{
    eddy_inc_font_size(e, 1);
}

void eddy_cmd_reset_font(Eddy *e, JSONValue)
{
    JSONValue appearance = json_get_default(&e->settings, "appearance", json_object());
    int       font_size = json_get_int(&appearance, "font_size", 20);
    app_set_font((App *) &eddy, e->font_path, font_size);
    eddy_set_message(e, "Font size %d", e->font_size);
}

void select_font_submit(ListBox *, ListBoxEntry selection)
{
    app_set_font((App *) &eddy, selection.string, eddy.font_size);
    eddy_set_message(&eddy, "Selected font '%.*s'", SV_ARG(selection.text));
}

void eddy_cmd_select_font(Eddy *e, JSONValue)
{
    ListBox *listbox = widget_new(ListBox);
    listbox->prompt = sv_from("Select font");
    listbox->submit = select_font_submit;
    StringList font_dirs = eddy_get_font_dirs(e);
    StringList fonts = { 0 };
    for (size_t ix = 0; ix < font_dirs.size; ++ix) {
        info("eddy_cmd_select_font: Directory %.*s", SV_ARG(font_dirs.strings[ix]));
        ErrorOrDirListing dir_maybe = fs_directory(font_dirs.strings[ix], DirOptionFiles);
        if (ErrorOrDirListing_is_error(dir_maybe)) {
            info("eddy_cmd_select_font: Invalid font directory %.*s", SV_ARG(font_dirs.strings[ix]));
            eddy_set_message(e, "Invalid font directory '%.*s'", SV_ARG(font_dirs.strings[ix]));
            continue;
        }
        DirListing dir = dir_maybe.value;
        for (size_t dix = 0; dix < dir.entries.size; ++dix) {
            DirEntry *entry = dir.entries.elements + dix;
            if (!sv_endswith(entry->name, SV(".ttf", 4)) && !sv_endswith(entry->name, SV(".TTF", 4))) {
                continue;
            }
            StringView path = sv_printf("%.*s/%.*s", SV_ARG(font_dirs.strings[ix]), SV_ARG(entry->name));
            char       buf[path.length + 1];
            Font       font = LoadFontEx(sv_cstr(path, buf), 20, NULL, 0);
            Vector2    w = MeasureTextEx(font, "W", 20, 2);
            Vector2    i = MeasureTextEx(font, "i", 20, 2);
            UnloadFont(font);
            if (w.x == i.x) {

                FT_Face face = { 0 };
                assert(FT_New_Face(ft_library, buf, 0, &face) == 0);
                if (FT_Get_Char_Index(face, 'A') == 0) {
                    continue;
                }

                da_append_ListBoxEntry(&listbox->entries, (ListBoxEntry) { .text = sv_copy(entry->name), .string = path });
                continue;
            }
        }
    }
    listbox_show(listbox);
}

void set_font_submit(InputBox *, StringView font_name)
{
    app_set_font((App *) &eddy, font_name, eddy.font_size);
    eddy_set_message(&eddy, "Selected font '%.*s'", SV_ARG(font_name));
}

void eddy_cmd_set_font(Eddy *e, JSONValue)
{
    InputBox *fontname_box = inputbox_create(SV("Font name", 9), set_font_submit);
    sb_append_sv(&fontname_box->text, eddy.font_path);
    inputbox_show(fontname_box);
}

void select_theme_submit(ListBox *, ListBoxEntry selection)
{
    eddy_load_theme(&eddy, selection.string);
    eddy_set_message(&eddy, "Selected theme '%.*s'", SV_ARG(selection.text));
}

void eddy_cmd_select_theme(Eddy *e, JSONValue)
{
    ListBox *listbox = widget_new(ListBox);
    listbox->prompt = sv_from("Select theme");
    listbox->submit = select_theme_submit;

    struct passwd *pw = getpwuid(getuid());
    StringBuilder  themes_dir = sb_createf("%s/.eddy", pw->pw_dir);
    MUST(Int, fs_assert_dir(themes_dir.view));
    sb_append_cstr(&themes_dir, "/themes");
    MUST(Int, fs_assert_dir(themes_dir.view));

    for (int loops = 1; loops <= 2; ++loops) {
        ErrorOrDirListing dir_maybe = fs_directory(themes_dir.view, DirOptionFiles);
        if (ErrorOrDirListing_is_error(dir_maybe)) {
            info("eddy_cmd_select_font: Invalid font directory %.*s", SV_ARG(themes_dir));
            eddy_set_message(e, "Invalid font directory '%.*s'", SV_ARG(themes_dir));
        }
        DirListing dir = dir_maybe.value;
        for (size_t dix = 0; dix < dir.entries.size; ++dix) {
            DirEntry *entry = dir.entries.elements + dix;
            if (!sv_endswith(entry->name, SV(".json", 5)) && !sv_endswith(entry->name, SV(".JSON", 5))) {
                continue;
            }
            StringView       path = sv_printf("%.*s/%.*s", SV_ARG(themes_dir), SV_ARG(entry->name));
            StringView       theme_name = (StringView) { entry->name.ptr, entry->name.length - 5 };
            StringView       json = MUST(StringView, read_file_by_name(path));
            ErrorOrJSONValue theme_maybe = json_decode(json);
            if (ErrorOrJSONValue_is_error(theme_maybe)) {
                continue;
            }
            theme_name = json_get_string(&theme_maybe.value, "name", theme_name);
            da_append_ListBoxEntry(
                &listbox->entries,
                (ListBoxEntry) {
                    .text = sv_copy(theme_name),
                    .string = sv_copy((StringView) { entry->name.ptr, entry->name.length - 5 }),
                });
            json_free(theme_maybe.value);
        }
        dl_free(dir);
        sv_free(themes_dir.view);
        themes_dir.view = (StringView) { 0 };
        sb_append_cstr(&themes_dir, EDDY_DATADIR "/themes");
    }
    sv_free(themes_dir.view);
    listbox_show(listbox);
}

void eddy_publish_diagnostics_handler(Eddy *eddy, JSONValue notif)
{
    Notification                     notification = notification_decode(&notif);
    OptionalPublishDiagnosticsParams params_maybe = PublishDiagnosticsParams_decode(notification.params);
    if (!params_maybe.has_value) {
        info("Could not decode textdocument/publishDiagnostics notification");
        return;
    }
    PublishDiagnosticsParams params = params_maybe.value;
    for (size_t ix = 0; ix < eddy->buffers.size; ++ix) {
        Buffer *buffer = eddy->buffers.elements + ix;
        if (sv_eq(params.uri, buffer_uri(buffer))) {
            size_t prev_num = buffer->diagnostics.size;
            da_free_Diagnostic(&buffer->diagnostics);
            buffer->diagnostics = params.diagnostics;
            params.diagnostics = (Diagnostics) { 0 };
            if (buffer->diagnostics.size != prev_num) {
                bool is_saved = buffer->version == buffer->saved_version;
                ++buffer->version;
                if (is_saved) {
                    buffer->saved_version = buffer->version;
                }
            }
            return;
        }
    }
    info("Received diagnostics for unknown document '%.*s'", SV_ARG(params.uri));
}

void eddy_query_close(Eddy *eddy)
{
    app_submit((App *) eddy, eddy, SV("eddy-quit", 9), json_null());
}

Eddy *eddy_create()
{
    app_state_read(&app_state);
    eddy.monitor = app_state.state[AS_MONITOR];
    eddy.handlers.init = (WidgetInit) eddy_init;
    return &eddy;
}

void eddy_init(Eddy *eddy)
{
    char const *project_dir = ".";
    int         ix;
    for (ix = 1; ix < eddy->argc; ++ix) {
        if ((strlen(eddy->argv[ix]) <= 2) || eddy->argv[ix][0] != '-' || eddy->argv[ix][1] != '-') {
            break;
        }
        StringView  option = sv_from(eddy->argv[ix] + 2);
        StringView  value = sv_from("true");
        char const *equals = strchr(eddy->argv[ix] + 2, '=');
        if (equals) {
            option = (StringView) { eddy->argv[ix] + 2, equals - eddy->argv[ix] - 2 };
            value = sv_from(equals + 1);
        }
        set_option(option, value);
    }
    log_init();

    if (ix < eddy->argc) {
        project_dir = eddy->argv[ix++];
    }

    if (FT_Init_FreeType(&ft_library)) {
        fatal("Could not initialize freetype");
    }

    Layout *editor_pane = layout_new(CO_HORIZONTAL);
    editor_pane->policy = SP_STRETCH;
    layout_add_widget(editor_pane, widget_new(Gutter));
    layout_add_widget(editor_pane, widget_new(Editor));
    Layout *main_area = layout_new(CO_VERTICAL);
    main_area->policy = SP_STRETCH;
    layout_add_widget(main_area, editor_pane);
    layout_add_widget(main_area, widget_new(StatusBar));
    layout_add_widget(main_area, widget_new(MiniBuffer));
    layout_add_widget((Layout *) eddy, (Widget *) main_area);
    eddy->editor = (Editor *) layout_find_by_draw_function((Layout *) eddy, (WidgetDraw) editor_draw);

    Mode *c_mode = (Mode *) widget_new_with_parent(CMode, eddy);
    da_append_Widget(&eddy->modes, (Widget *) c_mode);

    eddy_open_dir(eddy, sv_from(project_dir));
    for (; ix < eddy->argc; ++ix) {
        eddy_open_buffer(eddy, sv_from(eddy->argv[ix]));
    }
    if (eddy->buffers.size == 0) {
        eddy_new_buffer(eddy);
    }
    editor_select_buffer(eddy->editor, 0);

    widget_add_command(eddy, "eddy-force-quit", (WidgetCommandHandler) eddy_cmd_force_quit,
        (KeyCombo) { KEY_Q, KMOD_CONTROL | KMOD_SHIFT });
    widget_add_command(eddy, "eddy-quit", (WidgetCommandHandler) eddy_cmd_quit,
        (KeyCombo) { KEY_Q, KMOD_CONTROL });
    widget_add_command(eddy, "eddy-run-command", (WidgetCommandHandler) eddy_cmd_run_command,
        (KeyCombo) { KEY_P, KMOD_SUPER | KMOD_SHIFT });
    widget_add_command(eddy, "eddy-open-file", (WidgetCommandHandler) eddy_cmd_open_file,
        (KeyCombo) { KEY_O, KMOD_CONTROL });
    widget_add_command(eddy, "eddy-search-file", (WidgetCommandHandler) eddy_cmd_search_file,
        (KeyCombo) { KEY_O, KMOD_SUPER });
    widget_add_command(eddy, "eddy-shrink-font", (WidgetCommandHandler) eddy_cmd_shrink_font,
        (KeyCombo) { KEY_MINUS, KMOD_SUPER });
    widget_add_command(eddy, "eddy-enlarge-font", (WidgetCommandHandler) eddy_cmd_enlarge_font,
        (KeyCombo) { KEY_EQUAL, KMOD_SUPER });
    widget_add_command(eddy, "eddy-reset-font", (WidgetCommandHandler) eddy_cmd_reset_font,
        (KeyCombo) { KEY_ZERO, KMOD_SUPER });
    widget_add_command(eddy, "cmake-build", (WidgetCommandHandler) cmake_cmd_build,
        (KeyCombo) { KEY_F9, KMOD_NONE });
    widget_register(eddy, "lsp-textDocument/publishDiagnostics", (WidgetCommandHandler) eddy_publish_diagnostics_handler);
    widget_register(eddy, "display-message", (WidgetCommandHandler) eddy_cmd_set_message);
    widget_register(eddy, "eddy-select-font", (WidgetCommandHandler) eddy_cmd_select_font);
    widget_register(eddy, "eddy-set-font", (WidgetCommandHandler) eddy_cmd_set_font);
    widget_register(eddy, "eddy-select-theme", (WidgetCommandHandler) eddy_cmd_select_theme);
    widget_register(eddy, "show-message-box", (WidgetCommandHandler) eddy_cmd_display_messagebox);

    eddy->viewport.width = WINDOW_WIDTH;
    eddy->viewport.height = WINDOW_HEIGHT;
    eddy->classname = "Eddy";
    eddy->handlers.on_start = (WidgetOnStart) eddy_on_start;
    eddy->handlers.on_terminate = (WidgetOnTerminate) eddy_on_terminate;
    eddy->handlers.on_draw = (WidgetDraw) eddy_on_draw;
    eddy->handlers.process_input = (WidgetProcessInput) eddy_process_input;
    eddy->queryclose = (AppQueryClose) eddy_query_close;
    app_init((App *) eddy);
}

void eddy_on_start(Eddy *eddy)
{
    eddy->monitor = GetCurrentMonitor();
    eddy_load_font(eddy);
}

void eddy_on_terminate(Eddy *eddy)
{
    UnloadFont(eddy->font);
}

void eddy_process_input(Eddy *eddy)
{
    if (eddy->monitor != app_state.state[AS_MONITOR]) {
        app_state.state[AS_MONITOR] = eddy->monitor;
        app_state_write(&app_state);
    }
    app_process_input((App *) eddy);
}

void eddy_on_draw(Eddy *eddy)
{
    for (size_t ix = 0; ix < eddy->buffers.size; ++ix) {
        Buffer *buffer = eddy->buffers.elements + ix;
        if (buffer->indexed_version != buffer->version) {
            buffer_build_indices(buffer);
            for (size_t view_ix = 0; view_ix < eddy->editor->buffers.size; ++view_ix) {
                BufferView *view = eddy->editor->buffers.elements + view_ix;
                if (view->buffer_num == ix && buffer->mode && buffer->mode->handlers.on_draw) {
                    buffer->mode->handlers.on_draw((Widget *) buffer->mode);
                }
            }
        }
    }
    ClearBackground(colour_to_color(eddy->theme.editor.bg));
}

void eddy_read_settings(Eddy *eddy)
{
    json_free(eddy->settings);
    JSONValue settings = json_object();
    if (fs_file_exists(sv_from(EDDY_DATADIR "/settings.json"))) {
        StringView s = MUST(StringView, read_file_by_name(sv_from(EDDY_DATADIR "/settings.json")));
        JSONValue  system_wide = MUST(JSONValue, json_decode(s));
        sv_free(s);
        json_merge(&settings, system_wide);
        json_free(system_wide);
    }

    struct passwd *pw = getpwuid(getuid());
    StringView     eddy_fname = sv_printf("%s/.eddy", pw->pw_dir);
    MUST(Int, fs_assert_dir(eddy_fname));
    StringView user_settings_fname = sv_printf("%.*s/settings.json", SV_ARG(eddy_fname));
    sv_free(eddy_fname);
    if (fs_file_exists(user_settings_fname)) {
        StringView s = MUST(StringView, read_file_by_name(user_settings_fname));
        JSONValue  user = MUST(JSONValue, json_decode(s));
        sv_free(s);
        json_merge(&settings, user);
        json_free(user);
    }
    sv_free(user_settings_fname);

    if (fs_file_exists(SV(".eddy/settings.json", 19))) {
        StringView s = MUST(StringView, read_file_by_name(SV(".eddy/settings.json", 19)));
        JSONValue  prj = MUST(JSONValue, json_decode(s));
        sv_free(s);
        json_merge(&settings, prj);
        json_free(prj);
    }
    eddy->settings = settings;
    JSONValue appearance = json_get_default(&eddy->settings, "appearance", json_object());
    JSONValue theme_name = json_get_default(&appearance, "theme", json_string(SV("darcula", 7)));
    assert(theme_name.type == JSON_TYPE_STRING);
    eddy_load_theme(eddy, theme_name.string);
}

void eddy_load_font(Eddy *e)
{
    JSONValue appearance = json_get_default(&e->settings, "appearance", json_object());
    JSONValue default_font = json_string(SV("VictorMono-Medium.ttf", 21));
    JSONValue font = json_get_default(&appearance, "font", default_font);
    assert(font.type == JSON_TYPE_STRING);
    JSONValue font_size = json_get_default(&appearance, "font_size", json_int(20));
    assert(font_size.type == JSON_TYPE_INT);

    StringList font_dirs = eddy_get_font_dirs(e);
    for (int tries = 0; tries < 2; ++tries) {
        for (size_t ix = 0; ix < font_dirs.size; ++ix) {
            StringView path = sv_printf("%.*s/%.*s", SV_ARG(font_dirs.strings[ix]), SV_ARG(font.string));
            if (fs_file_exists(path) && !fs_is_directory(path)) {
                trace(EDIT, "Found font file %.*s", SV_ARG(path));
                app_set_font((App *) e, path, json_int_value(font_size));
                sv_free(path);
                return;
            }
            sv_free(path);
        }
        font = default_font;
    }
    fatal("Could not load font!");
}

void eddy_load_theme(Eddy *e, StringView theme_name)
{
    ErrorOrTheme theme_maybe = theme_load(theme_name);
    if (ErrorOrTheme_is_error(theme_maybe)) {
        info("Error loading theme: %s", Error_to_string(theme_maybe.error));
        eddy_set_message(e, "Error loading theme: %s", Error_to_string(theme_maybe.error));
        return;
    }
    e->theme = theme_maybe.value;
    for (size_t ix = 0; ix < e->buffers.size; ++ix) {
        Buffer *buffer = e->buffers.elements + ix;
        bool    is_saved = buffer->version == buffer->saved_version;
        ++buffer->version;
        buffer_build_indices(buffer);
        if (is_saved) {
            buffer->saved_version = buffer->version;
        }
    }
    if (e->handlers.resize) {
        e->handlers.resize((Widget *) e);
    }
}

void eddy_open_dir(Eddy *e, StringView dir)
{
    dir = fs_canonical(dir);
    MUST(Int, fs_assert_dir(dir));
    char buf[dir.length + 1];
    if (chdir(sv_cstr(dir, buf)) != 0) {
        fatal("Cannot open project directory '%.*s': Could not chdir(): %s", SV_ARG(dir), strerror(errno));
    }
    e->project_dir = dir;
    MUST(Int, fs_assert_dir(SV(".eddy", 5)));
    JSONValue prj = json_object();
    if (fs_file_exists(sv_from(".eddy/project.json"))) {
        StringView s = MUST(StringView, read_file_by_name(sv_from(".eddy/project.json")));
        prj = MUST(JSONValue, json_decode(s));
    }
    JSONValue source_dirs = json_get_default(&prj, "sources", json_array());
    assert(source_dirs.type == JSON_TYPE_ARRAY);
    if (json_len(&source_dirs) == 0) {
        sl_push(&e->source_dirs, SV(".", 1));
    } else {
        for (int ix = 0; ix < json_len(&source_dirs); ++ix) {
            JSONValue d = MUST_OPTIONAL(JSONValue, json_at(&source_dirs, ix));
            assert(d.type == JSON_TYPE_STRING);
            sl_push(&e->source_dirs, sv_copy(d.string));
        }
    }
    JSONValue cmake = json_get_default(&prj, "cmake", json_object());
    assert(cmake.type == JSON_TYPE_OBJECT);
    e->cmake.cmakelists = sv_copy(json_get_string(&cmake, "cmakelists", SV("CMakeLists.txt", 14)));
    e->cmake.build_dir = sv_copy(json_get_string(&cmake, "build", SV("build", 5)));
    eddy_read_settings(e);
    if (fs_file_exists(SV(".eddy/state", 11))) {
        StringView s = MUST(StringView, read_file_by_name(SV(".eddy/state", 11)));
        JSONValue  state = MUST(JSONValue, json_decode(s));
        JSONValue  files = json_get_default(&state, "files", json_array());
        assert(files.type == JSON_TYPE_ARRAY);
        for (int ix = 0; ix < json_len(&files); ++ix) {
            JSONValue f = MUST_OPTIONAL(JSONValue, json_at(&files, ix));
            assert(f.type == JSON_TYPE_STRING);
            MUST(Buffer, eddy_open_buffer(e, f.string));
        }
    }
}

ErrorOrBuffer eddy_open_buffer(Eddy *e, StringView file)
{
    assert(sv_not_empty(file));
    file = fs_relative(file, e->project_dir);
    for (size_t ix = 0; ix < e->buffers.size; ++ix) {
        Buffer *b = e->buffers.elements + ix;
        if (sv_eq(b->name, file)) {
            buffer_build_indices(b);
            RETURN(Buffer, b);
        }
    }
    return buffer_open(eddy_new_buffer(e), file);
}

Buffer *eddy_new_buffer(Eddy *e)
{
    Buffer *buffer;
    for (int ix = 0; ix < e->buffers.size; ++ix) {
        Buffer *b = e->buffers.elements + ix;
        if (sv_empty(b->name) && sv_empty(b->text.view)) {
            buffer_build_indices(b);
            b->buffer_ix = ix;
            return b;
        }
    }
    Buffer *b = da_append_Buffer(&e->buffers, (Buffer) { 0 });
    in_place_widget(Buffer, b, e);
    b->buffer_ix = (int) e->buffers.size - 1;
    buffer_build_indices(b);
    return b;
}

void eddy_close_buffer(Eddy *e, int buffer_num)
{
    Buffer *buffer = e->buffers.elements + buffer_num;
    buffer_close(buffer);
    if (buffer_num == e->buffers.size) {
        --e->buffers.size;
    }
}

void eddy_set_message(Eddy *e, char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    JSONValue msg = json_string(sv_vprintf(fmt, args));
    va_end(args);
    app_submit((App *) e, (Widget *) app, SV("display-message", 15), msg);
}

Mode *eddy_get_mode_for_buffer(Eddy *e, StringView buffer_name)
{
    for (size_t mode_ix = 0; mode_ix < e->modes.size; ++mode_ix) {
        Mode *mode = (Mode *) e->modes.elements + mode_ix;
        for (size_t ext_ix = 0; ext_ix < mode->filetypes.size; ++ext_ix) {
            if (sv_endswith(buffer_name, mode->filetypes.strings[ext_ix])) {
                return mode;
            }
        }
    }
    return NULL;
}
