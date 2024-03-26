/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __APP_THEME_H__
#define __APP_THEME_H__

#include <stdint.h>

#include <sv.h>

typedef struct {
    union {
        struct {
            unsigned char r; // Color red value
            unsigned char g; // Color green value
            unsigned char b; // Color blue value
            unsigned char a; // Color alpha value
        };
        unsigned char components[4];
        uint32_t      rgba;
    };
} Colour;

OPTIONAL(Colour);
ERROR_OR(Colour);

typedef struct {
    Colour bg;
    Colour fg;
} Colours;

typedef struct {
    StringView name;
    StringList scope;
    Colours    colours;
} TokenColour;

OPTIONAL(TokenColour);
ERROR_OR(TokenColour);
DA_WITH_NAME(TokenColour, TokenColours);

typedef struct {
    Colours      editor;
    Colours      selection;
    Colours      linehighlight;
    Colours      gutter;
    TokenColours token_colours;
} Theme;

ERROR_OR(Theme);

extern StringView   colours_to_string(Colours colours);
extern StringView   colour_to_rgb(Colour colour);
extern StringView   colour_to_hex(Colour colour);
extern ErrorOrTheme theme_load(StringView name);
extern OptionalInt  theme_index_for_scope(Theme *theme, StringView scope);

#endif /* __APP_THEME_H__ */
