/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __APP_WIDGET_H__
#define __APP_WIDGET_H__

#include <raylib.h>

#include <da.h>

typedef enum {
    TA_LEFT,
    TA_RIGHT,
    TA_CENTER,
} TextAlignment;

typedef enum {
    SP_ABSOLUTE = 0,
    SP_RELATIVE,
    SP_CHARACTERS,
    SP_CALCULATED,
    SP_STRETCH,
} SizePolicy;

typedef enum {
    CO_VERTICAL = 0,
    CO_HORIZONTAL,
} ContainerOrientation;

typedef struct {
    Rectangle viewport;
} Widget;

DA_WITH_NAME(Rectangle, Rectangles);
DA_STRUCT_WITH_NAME(Widget, Widget *, Widgets);

typedef struct {
    ContainerOrientation orientation;
    Widgets              widgets;
    Rectangles           outlines;
    Widget              *mouse_focus;
} WidgetContainer;

typedef struct {
    Widget          _widget;
    WidgetContainer container;
} Layout;

#endif /* __APP_WIDGET_H__ */
