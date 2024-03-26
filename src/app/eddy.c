/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "error_or.h"
#include "sv.h"
#include "theme.h"
#include <errno.h>
#include <pwd.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <app/cmake.h>
#include <app/eddy.h>
#include <app/listbox.h>
#include <app/minibuffer.h>
#include <app/palette.h>
#include <base/fs.h>
#include <base/io.h>
#include <base/json.h>
#include <base/options.h>
#include <lsp/lsp.h>
#include <lsp/schema/Diagnostic.h>
#include <lsp/schema/PublishDiagnosticsParams.h>

#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 768

void eddy_load_theme(Eddy *eddy);

AppState state = { 0 };
Eddy     eddy = { 0 };

LAYOUT_CLASS_DEF(StatusBar, sb);

void app_state_read(AppState *app_state)
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
        read(state_fd, app_state, sizeof(AppState));
        close(state_fd);
    } else {
        app_state_write(app_state);
    }
}

void app_state_write(AppState *app_state)
{
    struct passwd *pw = getpwuid(getuid());
    char const    *state_fname = TextFormat("%s/.eddy/state", pw->pw_dir);
    int            state_fd = open(state_fname, O_RDWR | O_CREAT | O_TRUNC, 0600);
    write(state_fd, app_state, sizeof(app_state));
    close(state_fd);
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

void sb_cursor_draw(Label *label)
{
    if (!label->parent->memo) {
        label->parent->memo = layout_find_by_draw_function((Layout *) label->parent->parent, (WidgetDraw) editor_draw);
    }
    Editor *editor = (Editor *) label->parent->memo;
    assert(editor);
    BufferView *view = editor->buffers.elements + eddy.editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    size_t      len = buffer->lines.elements[view->cursor_pos.line].line.length;
    if (view->selection != -1) {
        label->text = sv_from(TextFormat("row:col %d:%d sel: %zu-%zu len %zu pos %zu col %d clk %d vp %d:%d",
            view->cursor_pos.line + 1, view->cursor_pos.column + 1,
            view->selection, view->cursor,
            len, view->cursor, view->cursor_col, editor->num_clicks,
            view->left_column, view->top_line));
    } else {
        label->text = sv_from(TextFormat("row:col %d:%d len %zu pos %zu col %d clk %d vp %d:%d",
            view->cursor_pos.line + 1, view->cursor_pos.column + 1,
            len, view->cursor, view->cursor_col, editor->num_clicks,
            view->left_column, view->top_line));
    }
    label_draw(label);
}

void sb_last_key_draw(Label *label)
{
    label->text = sv_from(eddy.last_key);
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
    widget_draw_rectangle(sb, 0, 0, 0, 0, RAYWHITE);
}

void sb_init(StatusBar *status_bar)
{
    status_bar->orientation = CO_HORIZONTAL;
    status_bar->policy = SP_CHARACTERS;
    status_bar->policy_size = 1.0f;
    status_bar->handlers.on_draw = (WidgetOnDraw) sb_on_draw;
    layout_add_widget((Layout *) status_bar, (Widget *) widget_new_with_policy(Spacer, SP_CHARACTERS, 1));
    Label *file_name = (Label *) widget_new(Label);
    file_name->policy_size = 64;
    file_name->color = DARKGRAY;
    file_name->handlers.draw = (WidgetDraw) sb_file_name_draw;
    layout_add_widget((Layout *) status_bar, (Widget *) file_name);
    layout_add_widget((Layout *) status_bar, (Widget *) widget_new(Spacer));
    Label *cursor = widget_new(Label);
    cursor->policy_size = 30;
    cursor->color = DARKGRAY;
    cursor->handlers.draw = (WidgetDraw) sb_cursor_draw;
    layout_add_widget((Layout *) status_bar, (Widget *) cursor);
    Label *last_key = widget_new(Label);
    last_key->policy_size = 16;
    last_key->color = DARKGRAY;
    last_key->handlers.draw = (WidgetDraw) sb_last_key_draw;
    layout_add_widget((Layout *) status_bar, (Widget *) last_key);
    Label *fps = widget_new(Label);
    fps->policy_size = 4;
    fps->handlers.draw = (WidgetDraw) sb_fps_draw;
    layout_add_widget((Layout *) status_bar, (Widget *) fps);
}

APP_CLASS_DEF(Eddy, eddy);

void eddy_quit(Eddy *eddy)
{
    JSONValue state = json_object();
    JSONValue files = json_array();
    for (size_t ix = 0; ix < eddy->buffers.size; ++ix) {
        if (sv_not_empty(eddy->buffers.elements[ix].name)) {
            StringView rel = fs_relative(eddy->buffers.elements[ix].name, eddy->project_dir);
            json_append(&files, json_string(rel));
        }
    }
    json_set(&state, "files", files);
    MUST(Size, write_file_by_name(sv_from(".eddy/state"), json_encode(state)));
    eddy->quit = true;
}

void eddy_are_you_sure_handler(ListBox *are_you_sure, QueryOption selection)
{
    if (selection == QueryOptionYes) {
        eddy_quit(&eddy);
    }
}

void eddy_cmd_quit(Eddy *eddy, JSONValue unused)
{
    bool has_modified_buffers = false;
    for (size_t ix = 0; ix < eddy->buffers.size; ++ix) {
        Buffer *buffer = da_element_Buffer(&eddy->buffers, ix);
        if (buffer->saved_version < buffer->version) {
            has_modified_buffers = true;
            break;
        }
    }
    StringView prompt = SV("Are you sure you want to quit?", 30);
    if (has_modified_buffers) {
        prompt = SV("There are modified files. Are you sure you want to quit?", 56);
    }
    ListBox *are_you_sure = listbox_create_query(prompt, eddy_are_you_sure_handler, QueryOptionYesNo);
    listbox_show(are_you_sure);
}

void run_command_submit(ListBox *listbox, ListBoxEntry selection)
{
    WidgetCommand *cmd = (WidgetCommand *) selection.payload;
    app_submit((App *) &eddy, cmd->owner, cmd->command, json_null());
    eddy_set_message(&eddy, "Selected command '%.*s'", SV_ARG(cmd->command));
}

void eddy_cmd_run_command(Eddy *eddy, JSONValue unused)
{
    ListBox *listbox = widget_new(ListBox);
    listbox->prompt = sv_from("Select commmand");
    listbox->submit = run_command_submit;
    for (Widget *w = eddy->focus; w; w = w->parent) {
        for (size_t cix = 0; cix < w->commands.size; ++cix) {
            WidgetCommand *command = w->commands.elements + cix;
            da_append_ListBoxEntry(&listbox->entries, (ListBoxEntry) { command->command, command });
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

void eddy_cmd_open_file(Eddy *eddy, JSONValue unused)
{
    ListBox *listbox = file_selector_create(SV("Select file", 11), eddy_open_fs_handler, FSFile);
    listbox_show(listbox);
}

void free_search_entry(ListBox *listbox, ListBoxEntry entry)
{
    sv_free(sv_from((char const *) entry.payload));
    sv_free(entry.text);
}

void file_search_submit(ListBox *listbox, ListBoxEntry selection)
{
    StringView    filename = sv_from((char const *) selection.payload);
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
            da_append_ListBoxEntry(&listbox->entries, (ListBoxEntry) { label, (void *) fullpath.ptr });
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

void eddy_cmd_search_file(Eddy *eddy, JSONValue unused)
{
    ListBox *listbox = widget_new(ListBox);
    listbox->submit = file_search_submit;
    listbox->free_entry = free_search_entry;
    for (size_t ix = 0; ix < eddy->source_dirs.size; ++ix) {
        ErrorOrInt error_maybe = fill_search_listbox(listbox, eddy->source_dirs.strings[ix]);
        if (ErrorOrInt_is_error(error_maybe)) {
            eddy_set_message(eddy, "Could not list source directory '%.*': %s", SV_ARG(eddy->source_dirs.strings[ix]), Error_to_string(error_maybe.error));
            listbox_free(listbox);
            return;
        }
    }
    listbox_show(listbox);
}

void eddy_cmd_set_message(Eddy *eddy, JSONValue message)
{
    assert(message.type == JSON_TYPE_STRING);
    minibuffer_set_message_sv(message.string);
}

void eddy_cmd_display_messagebox(Eddy *eddy, JSONValue message)
{
    assert(message.type == JSON_TYPE_STRING);
    ListBox *box = messagebox_create(message.string);
    listbox_show(box);
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
            da_free_Diagnostic(&buffer->diagnostics);
            buffer->diagnostics = params.diagnostics;
            params.diagnostics = (Diagnostics) { 0 };
            ++buffer->version;

            trace(CAT_LSP, "Diagnostics for '%.*s':", SV_ARG(params.uri));
            for (size_t dix = 0; dix < buffer->diagnostics.size; ++dix) {
                Diagnostic *d = buffer->diagnostics.elements + dix;
                trace(CAT_LSP, "%.*s:%d:%d %.*s", SV_ARG(buffer->name), d->range.start.line, d->range.start.character, SV_ARG(d->message));
            }
            trace(CAT_LSP, "----");

            return;
        }
    }
    info("Received diagnostics for unknown document '%.*s'", SV_ARG(params.uri));
}

Eddy *eddy_create()
{
    app_state_read(&state);
    eddy.monitor = state.state[AS_MONITOR];
    eddy.handlers.init = (WidgetInit) eddy_init;
    return &eddy;
}

void eddy_init(Eddy *eddy)
{
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
    eddy_open_dir(eddy, sv_from(project_dir));
    for (; ix < eddy->argc; ++ix) {
        eddy_open_buffer(eddy, sv_from(eddy->argv[ix]));
    }
    if (eddy->buffers.size == 0) {
        eddy_new_buffer(eddy);
    }
    editor_select_buffer(eddy->editor, 0);

    widget_add_command(eddy, "eddy-quit", (WidgetCommandHandler) eddy_cmd_quit,
        (KeyCombo) { KEY_Q, KMOD_CONTROL });
    widget_add_command(eddy, "eddy-run-command", (WidgetCommandHandler) eddy_cmd_run_command,
        (KeyCombo) { KEY_X, KMOD_SUPER });
    widget_add_command(eddy, "eddy-open-file", (WidgetCommandHandler) eddy_cmd_open_file,
        (KeyCombo) { KEY_O, KMOD_CONTROL });
    widget_add_command(eddy, "eddy-search-file", (WidgetCommandHandler) eddy_cmd_search_file,
        (KeyCombo) { KEY_O, KMOD_SUPER });
    widget_add_command(eddy, "cmake-build", (WidgetCommandHandler) cmake_cmd_build,
        (KeyCombo) { KEY_F9, KMOD_NONE });
    widget_register(eddy, "lsp-textDocument/publishDiagnostics", (WidgetCommandHandler) eddy_publish_diagnostics_handler);
    widget_register(eddy, "display-message", (WidgetCommandHandler) eddy_cmd_set_message);
    widget_register(eddy, "show-message-box", (WidgetCommandHandler) eddy_cmd_display_messagebox);

    eddy->viewport.width = WINDOW_WIDTH;
    eddy->viewport.height = WINDOW_HEIGHT;
    eddy->classname = "Eddy";
    eddy->handlers.on_start = (WidgetOnStart) eddy_on_start;
    eddy->handlers.on_terminate = (WidgetOnTerminate) eddy_on_terminate;
    eddy->handlers.on_draw = (WidgetDraw) eddy_on_draw;
    eddy->handlers.process_input = (WidgetProcessInput) eddy_process_input;
    app_init((App *) eddy);
}

void eddy_on_start(Eddy *eddy)
{
    eddy->monitor = GetCurrentMonitor();
    eddy_load_font(eddy);
    eddy_load_theme(eddy);
}

void eddy_on_terminate(Eddy *eddy)
{
    UnloadFont(eddy->font);
}

void eddy_process_input(Eddy *eddy)
{
    if (eddy->monitor != state.state[AS_MONITOR]) {
        state.state[AS_MONITOR] = eddy->monitor;
        app_state_write(&state);
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
                if (view->buffer_num == ix && view->mode && view->mode->handlers.on_draw) {
                    view->mode->handlers.on_draw(view->mode);
                }
            }
        }
    }
    ClearBackground(palettes[PALETTE_DARK][PI_BACKGROUND]);
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
}

void eddy_load_font(Eddy *eddy)
{
    JSONValue appearance = json_get_default(&eddy->settings, "appearance", json_object());
    JSONValue directories = json_get_default(&appearance, "font_directories", json_array());
    if (directories.type == JSON_TYPE_STRING) {
        JSONValue dirs = json_array();
        json_append(&dirs, json_copy(directories));
        directories = dirs;
    }
    assert(directories.type == JSON_TYPE_ARRAY);
    json_append(&directories, json_string(sv_from(EDDY_DATADIR "/fonts")));
    JSONValue default_font = json_string(SV("VictorMono-Medium.ttf", 21));
    JSONValue font = json_get_default(&appearance, "font", default_font);
    assert(font.type == JSON_TYPE_STRING);
    JSONValue font_size = json_get_default(&appearance, "font_size", json_int(20));
    assert(font_size.type == JSON_TYPE_INT);

    struct passwd *pw = getpwuid(getuid());
    StringBuilder  d = { 0 };
    for (int tries = 0; tries < 2; ++tries) {
        for (size_t ix = 0; ix < json_len(&directories); ++ix) {
            JSONValue dir = MUST_OPTIONAL(JSONValue, json_at(&directories, ix));
            assert(dir.type == JSON_TYPE_STRING);
            d.length = 0;
            sb_append_sv(&d, dir.string);
            sb_replace_all(&d, SV("${HOME}", 7), sv_from(pw->pw_dir));
            sb_replace_all(&d, SV("${EDDY_DATADIR}", 15), sv_from(EDDY_DATADIR));
            StringView path = sv_printf("%.*s/%.*s", SV_ARG(d.view), SV_ARG(font.string));
            if (fs_file_exists(path) && !fs_is_directory(path)) {
                trace(CAT_EDIT, "Found font file %.*s", SV_ARG(path));
                char buf[path.length + 1];
                eddy->font = LoadFontEx(sv_cstr(path, buf), json_int_value(font_size), NULL, 0);
                sv_free(d.view);
                sv_free(path);
                return;
            }
            printf("Font file %.*s does not exist\n", SV_ARG(path));
            sv_free(path);
        }
        font = default_font;
    }
    fatal("Could not load font!");
}

void print_colours(char const *label, Colours colours)
{
    StringView s = colours_to_string(colours);
    printf("%s: %.*s\n", label, SV_ARG(s));
    sv_free(s);
}

void eddy_load_theme(Eddy *eddy)
{
    JSONValue    appearance = json_get_default(&eddy->settings, "appearance", json_object());
    JSONValue    theme_name = json_get_default(&appearance, "theme", json_string(SV("darcula", 7)));
    ErrorOrTheme theme_maybe = theme_load(theme_name.string);
    if (ErrorOrTheme_is_error(theme_maybe)) {
        info("Error loading theme: %s", Error_to_string(theme_maybe.error));
        eddy_set_message(eddy, "Error loading theme: %s", Error_to_string(theme_maybe.error));
        return;
    }
    eddy->theme = theme_maybe.value;
    print_colours("editor", eddy->theme.editor);
    print_colours("selection", eddy->theme.selection);
    print_colours("line highlight", eddy->theme.linehighlight);
    print_colours("gutter", eddy->theme.gutter);
    for (int ix = 0; ix < eddy->theme.token_colours.size; ++ix) {
        StringView s = colours_to_string(eddy->theme.token_colours.elements[ix].colours);
        printf("%.*s: %.*s\n", SV_ARG(eddy->theme.token_colours.elements[ix].name), SV_ARG(s));
        sv_free(s);
    }
}

void eddy_open_dir(Eddy *eddy, StringView dir)
{
    dir = fs_canonical(dir);
    MUST(Int, fs_assert_dir(dir));
    char buf[dir.length + 1];
    if (chdir(sv_cstr(dir, buf)) != 0) {
        fatal("Cannot open project directory '%.*s': Could not chdir(): %s", SV_ARG(dir), strerror(errno));
    }
    eddy->project_dir = dir;
    MUST(Int, fs_assert_dir(SV(".eddy", 5)));
    if (fs_file_exists(SV(".eddy/state", 11))) {
        StringView s = MUST(StringView, read_file_by_name(SV(".eddy/state", 11)));
        JSONValue  state = MUST(JSONValue, json_decode(s));
        JSONValue  files = json_get_default(&state, "files", json_array());
        assert(files.type == JSON_TYPE_ARRAY);
        for (int ix = 0; ix < json_len(&files); ++ix) {
            JSONValue f = MUST_OPTIONAL(JSONValue, json_at(&files, ix));
            assert(f.type == JSON_TYPE_STRING);
            MUST(Buffer, eddy_open_buffer(eddy, f.string));
        }
    }
    JSONValue prj = json_object();
    if (fs_file_exists(sv_from(".eddy/project.json"))) {
        StringView s = MUST(StringView, read_file_by_name(sv_from(".eddy/project.json")));
        prj = MUST(JSONValue, json_decode(s));
    }
    JSONValue source_dirs = json_get_default(&prj, "sources", json_array());
    assert(source_dirs.type == JSON_TYPE_ARRAY);
    if (json_len(&source_dirs) == 0) {
        sl_push(&eddy->source_dirs, SV(".", 1));
    } else {
        for (int ix = 0; ix < json_len(&source_dirs); ++ix) {
            JSONValue d = MUST_OPTIONAL(JSONValue, json_at(&source_dirs, ix));
            assert(d.type == JSON_TYPE_STRING);
            sl_push(&eddy->source_dirs, sv_copy(d.string));
        }
    }
    JSONValue cmake = json_get_default(&prj, "cmake", json_object());
    assert(cmake.type == JSON_TYPE_OBJECT);
    eddy->cmake.cmakelists = sv_copy(json_get_string(&cmake, "cmakelists", SV("CMakeLists.txt", 14)));
    eddy->cmake.build_dir = sv_copy(json_get_string(&cmake, "build", SV("build", 5)));
    eddy_read_settings(eddy);
}

ErrorOrBuffer eddy_open_buffer(Eddy *eddy, StringView file)
{
    assert(sv_not_empty(file));
    file = fs_relative(file, eddy->project_dir);
    for (size_t ix = 0; ix < eddy->buffers.size; ++ix) {
        Buffer *b = eddy->buffers.elements + ix;
        if (sv_eq(b->name, file)) {
            buffer_build_indices(b);
            RETURN(Buffer, b);
        }
    }
    return buffer_open(eddy_new_buffer(eddy), file);
}

Buffer *eddy_new_buffer(Eddy *eddy)
{
    Buffer *buffer;
    for (size_t ix = 0; ix < eddy->buffers.size; ++ix) {
        Buffer *b = eddy->buffers.elements + ix;
        if (sv_empty(b->name) && sv_empty(b->text.view)) {
            buffer_build_indices(b);
            b->buffer_ix = ix;
            return b;
        }
    }
    Buffer *b = da_append_Buffer(&eddy->buffers, (Buffer) { 0 });
    in_place_widget(Buffer, b, eddy);
    b->buffer_ix = eddy->buffers.size - 1;
    buffer_build_indices(b);
    return b;
}

void eddy_close_buffer(Eddy *eddy, int buffer_num)
{
    Buffer *buffer = eddy->buffers.elements + buffer_num;
    buffer_close(buffer);
    if (buffer_num == eddy->buffers.size) {
        --eddy->buffers.size;
    }
}

void eddy_set_message(Eddy *eddy, char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    JSONValue msg = json_string(sv_vprintf(fmt, args));
    va_end(args);
    app_submit((App *) eddy, (Widget *) app, SV("display-message", 15), msg);
}
