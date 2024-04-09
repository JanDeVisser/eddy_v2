/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __APP_THEME_H__
#define __APP_THEME_H__

#include <raylib.h>
#include <stdint.h>

#include <base/sv.h>
#include <base/token.h>
#include <lsp/schema/SemanticTokenTypes.h>

typedef struct {
    union {
        Color         color;
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

OPTIONAL(Colours);
ERROR_OR(Colours);

typedef struct {
    StringView name;
    StringList scope;
    Colours    colours;
} TokenColour;

OPTIONAL(TokenColour);
ERROR_OR(TokenColour);
DA_WITH_NAME(TokenColour, TokenColours);

typedef struct {
    SemanticTokenTypes token_type;
    Colours            colours;
} SemanticTokenColour;

OPTIONAL(SemanticTokenColour);
ERROR_OR(SemanticTokenColour);
DA_WITH_NAME(SemanticTokenColour, SemanticTokenColours);

typedef struct {
    TokenKind kind;
//    TokenCode code;
    int       theme_index;
} TokenThemeMapping;

OPTIONAL(TokenThemeMapping);
DA_WITH_NAME(TokenThemeMapping, TokenThemeMappings);

typedef struct {
    int semantic_index;
    int semantic_theme_index;
    int token_theme_index;
} SemanticMapping;

DA_WITH_NAME(SemanticMapping, SemanticMappings);

typedef struct {
    Colours              editor;
    Colours              selection;
    Colours              linehighlight;
    Colours              gutter;
    TokenColours         token_colours;
    SemanticTokenColours semantic_colours;
    TokenThemeMappings   token_mappings;
    SemanticMappings     semantic_mappings;
} Theme;

ERROR_OR(Theme);

extern StringView      colours_to_string(Colours colours);
extern StringView      colour_to_rgb(Colour colour);
extern StringView      colour_to_hex(Colour colour);
extern ErrorOrTheme    theme_load(StringView name);
extern OptionalInt     theme_index_for_scope(Theme *theme, StringView scope);
extern OptionalColours theme_token_colours(Theme *theme, Token t);
extern OptionalColours theme_semantic_colours(Theme *theme, int semantic_index);
extern void            theme_map_semantic_type(Theme *theme, int semantic_index, SemanticTokenTypes type);

static Color colour_to_color(Colour colour)
{
    return colour.color;
}

#endif /* __APP_THEME_H__ */
