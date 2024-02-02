/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <eddy.h>
#include <io.h>
#include <palette.h>

Eddy eddy = { 0 };

DECLARE_SHARED_ALLOCATOR(eddy);

LAYOUT_CLASS_DEF(StatusBar, sb);

void sb_file_name_draw(Label *label)
{
    if (!label->parent->memo) {
        label->parent->memo = layout_find_by_draw_function((Layout *) label->parent->parent, (WidgetDraw) editor_draw);
    }
    Editor *editor = (Editor *) label->parent->memo;
    assert(editor);
    BufferView *view = editor->buffers.elements + eddy.editor->current_buffer;
    Buffer     *buffer = eddy.buffers.elements + view->buffer_num;
    label->text = buffer->name;
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
    if (view->selection != -1) {
        label->text = sv_from(TextFormat("%4d:%d %zu-%zu", view->cursor_pos.line + 1, view->cursor_pos.column + 1, view->selection, view->cursor));
    } else {
        label->text = sv_from(TextFormat("%4d:%d %zu", view->cursor_pos.line + 1, view->cursor_pos.column + 1, view->cursor));
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

void sb_init(StatusBar *status_bar)
{
    status_bar->orientation = CO_HORIZONTAL;
    status_bar->policy = SP_CHARACTERS;
    status_bar->policy_size = 1.0f;
    layout_add_widget((Layout *) status_bar, widget_new_with_policy(Spacer, SP_CHARACTERS, 1));
    Label *file_name = (Label *) widget_new(Label);
    file_name->policy_size = 64;
    file_name->handlers.draw = (WidgetDraw) sb_file_name_draw;
    layout_add_widget((Layout *) status_bar, (Widget *) file_name);
    layout_add_widget((Layout *) status_bar, widget_new(Spacer));
    Label *cursor = (Label *) widget_new(Label);
    cursor->policy_size = 16;
    cursor->handlers.draw = (WidgetDraw) sb_cursor_draw;
    layout_add_widget((Layout *) status_bar, (Widget *) cursor);
    Label *last_key = (Label *) widget_new(Label);
    last_key->policy_size = 16;
    last_key->handlers.draw = (WidgetDraw) sb_last_key_draw;
    layout_add_widget((Layout *) status_bar, (Widget *) last_key);
    Label *fps = (Label *) widget_new(Label);
    fps->policy_size = 4;
    fps->handlers.draw = (WidgetDraw) sb_fps_draw;
    layout_add_widget((Layout *) status_bar, (Widget *) fps);
}

WIDGET_CLASS_DEF(MessageLine, message_line);

void message_line_init(MessageLine *message_line)
{
    message_line->policy = SP_CHARACTERS;
    message_line->policy_size = 1.0f;
    message_line->message = sv_null();
}

void message_line_resize(MessageLine *message_line)
{
}

void message_line_draw(MessageLine *message_line)
{
    widget_draw_rectangle(message_line, 0, 0, message_line->viewport.width, message_line->viewport.height, palettes[PALETTE_DARK][PI_BACKGROUND]);
    if (!sv_empty(message_line->message)) {
        widget_render_text(message_line, 0, 0, message_line->message, palettes[PALETTE_DARK][PI_DEFAULT]);
    }
}

void message_line_process_input(MessageLine *message_line)
{
}

APP_CLASS_DEF(Eddy, eddy);

void eddy_init(Eddy *eddy)
{
    Layout *editor_pane = layout_new(CO_HORIZONTAL);
    editor_pane->policy = SP_STRETCH;
    layout_add_widget(editor_pane, widget_new(Gutter));
    layout_add_widget(editor_pane, widget_new(Editor));

    Layout *main_area = layout_new(CO_VERTICAL);
    main_area->policy = SP_STRETCH;
    layout_add_widget(main_area, (Widget *) editor_pane);
    Widget *sb = widget_new(StatusBar);
    layout_add_widget(main_area, widget_new(StatusBar));
    layout_add_widget(main_area, widget_new(MessageLine));

    layout_add_widget((Layout *) eddy, (Widget *) main_area);
    eddy->editor = (Editor *) layout_find_by_draw_function((Layout *) eddy, (WidgetDraw) editor_draw);
    if (eddy->argc > 1) {
        for (int ix = 1; ix < eddy->argc; ++ix) {
            eddy_open_buffer(eddy, sv_from(app->argv[ix]));
        }
    } else {
        editor_new(eddy->editor);
    }
    editor_select_buffer(eddy->editor, 0);
    eddy->handlers.on_draw = (WidgetDraw) eddy_on_draw;
    eddy->focus = (Widget *) eddy->editor;
    app_init((App *) eddy);
}

void eddy_on_draw(Eddy *eddy)
{
    for (size_t ix = 0; ix < eddy->buffers.size; ++ix) {
        Buffer *buffer = eddy->buffers.elements + ix;
        if (buffer->rebuild_needed) {
            buffer_build_indices(buffer);
        }
    }
}

void eddy_open_buffer(Eddy *eddy, StringView file)
{
    Buffer buffer = { 0 };
    buffer.name = file;
    buffer.text.view = MUST(StringView, read_file_by_name(file));
    buffer.rebuild_needed = true;
    da_append_Buffer(&eddy->buffers, buffer);
}

void eddy_set_message(Eddy *eddy, StringView message)
{
    MessageLine *message_line = (MessageLine *) layout_find_by_draw_function((Layout *) eddy, (WidgetDraw) message_line_draw);
    assert(message_line);
    if (!sv_empty(message_line->message)) {
        sv_free(message_line->message);
    }
    message_line->message = message;
}

void eddy_clear_message(Eddy *eddy)
{
    MessageLine *message_line = (MessageLine *) layout_find_by_draw_function((Layout *) eddy, (WidgetDraw) message_line_draw);
    assert(message_line);
    if (!sv_empty(message_line->message)) {
        sv_free(message_line->message);
    }
}
