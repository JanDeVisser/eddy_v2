/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <app/theme.h>
#include <base/fs.h>
#include <base/io.h>

DA_IMPL(TokenColour);
DA_IMPL(SemanticTokenColour);
DA_IMPL(TokenThemeMapping);
DA_IMPL(SemanticMapping);

ErrorOrColour colour_parse_hex_color(StringView color, int prefixlen, int num_components)
{
    Colour ret = { 0 };
    ret.color.a = 0xFF;
    char buf[3];
    buf[2] = 0;
    assert(color.length == prefixlen + num_components * 2);
    for (size_t ix = 0; ix < num_components; ++ix) {
        assert(color.ptr[2 * ix + prefixlen] != 0);
        memcpy(buf, color.ptr + prefixlen + 2 * ix, 2);
        char         *endptr;
        unsigned long c = strtoul(buf, &endptr, 16);
        if (*endptr != 0) {
            ERROR(Colour, ParserError, 0, "Could not parse hex color string '%.*s'", SV_ARG(color));
        }
        assert(c < 256);
        ret.components[ix] = (unsigned char) c;
    }
    RETURN(Colour, ret);
}

ErrorOrColour colour_parse_color(StringView color)
{
    if (color.length == 7 && color.ptr[0] == '#') {
        return colour_parse_hex_color(color, 1, 3);
    }
    if (color.length == 9 && color.ptr[0] == '#') {
        return colour_parse_hex_color(color, 1, 4);
    }
    if (color.length == 10 && strncmp(color.ptr, "0x", 2) == 0) {
        return colour_parse_hex_color(color, 2, 4);
    }
    if (color.length == 10 && strncmp(color.ptr, "0X", 2) == 0) {
        return colour_parse_hex_color(color, 2, 4);
    }
    StringView s = sv_strip(color);
    if (sv_startswith(s, SV("rgb(", 4)) || sv_startswith(color, SV("RGB(", 4))) {
        StringView s = sv_lchop(color, 4);
        if (!sv_endswith(s, SV(")", 1))) {
            ERROR(Colour, ParserError, 0, "Could not parse hex color string '%.*s'", SV_ARG(color));
        }
        s = sv_rchop(s, 1);
        StringList components = sv_split_by_whitespace(s);
        if (components.size != 3) {
            ERROR(Colour, ParserError, 0, "Could not parse hex color string '%.*s'", SV_ARG(color));
        }
        Colour ret = { 0 };
        ret.color.a = 0xFF;
        for (size_t ix = 0; ix < 4; ++ix) {
            IntegerParseResult res = sv_parse_u8(components.strings[ix]);
            if (!res.success) {
                ERROR(Colour, ParserError, 0, "Could not parse hex color string '%.*s'", SV_ARG(color));
            }
            ret.components[ix] = res.integer.u8;
        }
    }
    ERROR(Colour, ParserError, 0, "Could not parse color string '%.*s'", SV_ARG(color));
}

StringView colour_to_rgb(Colour colour)
{
    if (colour.color.a < 0xFF) {
        int perc = (int) (((float) colour.color.a) / 255.0) * 100.0;
        return sv_printf("rgb(%d %d %d / %d%%)", colour.color.r, colour.color.g, colour.color.b, perc);
    }
    return sv_printf("rgb(%d %d %d)", colour.color.r, colour.color.g, colour.color.b);
}

StringView colour_to_hex(Colour colour)
{
    return sv_printf("#%02x%02x%02x%02x", colour.color.r, colour.color.g, colour.color.b, colour.color.a);
}

StringView colours_to_string(Colours colours)
{
    StringView bg = colour_to_hex(colours.bg);
    StringView fg = colour_to_hex(colours.fg);
    StringView ret = sv_printf("bg: %.*s fg: %.*s", SV_ARG(bg), SV_ARG(fg));
    sv_free(fg);
    sv_free(bg);
    return ret;
}

ErrorOrUChar colour_decode_component(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN(UChar, 0);
    }
    if (json.value.type != JSON_TYPE_INT) {
        ERROR(UChar, ParserError, 0, "Color component value must be integer");
    }
    OptionalInteger i = integer_coerce_to(json.value.int_number, U8);
    if (!i.has_value) {
        ERROR(UChar, ParserError, 0, "Invalid integer color component value");
    }
    RETURN(UChar, i.value.u8);
}

ErrorOrColour colour_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN(Colour, (Colour) { 0 });
    }
    switch (json.value.type) {
    case JSON_TYPE_STRING:
        return colour_parse_color(json.value.string);
    case JSON_TYPE_INT: {
        Colour          ret = { 0 };
        OptionalInteger i = integer_coerce_to(json.value.int_number, U32);
        if (!i.has_value) {
            ERROR(Colour, ParserError, 0, "Could not convert integer color value");
        }
        ret.rgba = i.value.u32;
        RETURN(Colour, ret);
    }
    case JSON_TYPE_ARRAY: {
        if (json_len(&json.value) != 4) {
            ERROR(Colour, ParserError, 0, "Could not convert array color value");
        }
        Colour ret = { 0 };
        for (size_t ix = 0; ix < 4; ++ix) {
            ret.components[ix] = TRY_TO(UChar, Colour, colour_decode_component(json_at(&json.value, ix)));
        }
        RETURN(Colour, ret);
    }
    case JSON_TYPE_OBJECT: {
        Colour ret = { 0 };
        ret.color.r = TRY_TO(UChar, Colour, colour_decode_component(json_get(&json.value, "r")));
        if (json_has(&json.value, "red")) {
            ret.color.r = TRY_TO(UChar, Colour, colour_decode_component(json_get(&json.value, "red")));
        }
        ret.color.g = TRY_TO(UChar, Colour, colour_decode_component(json_get(&json.value, "g")));
        if (json_has(&json.value, "green")) {
            ret.color.r = TRY_TO(UChar, Colour, colour_decode_component(json_get(&json.value, "green")));
        }
        ret.color.b = TRY_TO(UChar, Colour, colour_decode_component(json_get(&json.value, "b")));
        if (json_has(&json.value, "blue")) {
            ret.color.b = TRY_TO(UChar, Colour, colour_decode_component(json_get(&json.value, "blue")));
        }
        ret.color.a = 0xFF;
        if (json_has(&json.value, "a")) {
            ret.color.a = TRY_TO(UChar, Colour, colour_decode_component(json_get(&json.value, "a")));
        }
        if (json_has(&json.value, "alpha")) {
            ret.color.a = TRY_TO(UChar, Colour, colour_decode_component(json_get(&json.value, "alpha")));
        }
        RETURN(Colour, ret);
    }
    default:
        ERROR(Colour, ParserError, 0, "Could not convert color value");
    }
}

ErrorOrTokenColour token_colour_decode(JSONValue *json)
{
    TokenColour       ret = { 0 };
    OptionalJSONValue name = json_get(json, "name");
    if (name.has_value) {
        if (name.value.type != JSON_TYPE_STRING) {
            ERROR(TokenColour, IOError, 0, "'tokenColor' entry name must be a string");
        }
        ret.name = sv_copy(name.value.string);
    }
    OptionalJSONValue scope = json_get(json, "scope");
    if (scope.has_value) {
        if (scope.value.type != JSON_TYPE_STRING && scope.value.type != JSON_TYPE_ARRAY) {
            ERROR(TokenColour, IOError, 0, "'tokenColor' entry scope value must be a string");
        }
        if (sv_empty(ret.name)) {
            if (scope.value.type == JSON_TYPE_STRING) {
                ret.name = sv_copy(scope.value.string);
            } else if (scope.value.type == JSON_TYPE_ARRAY && json_len(&scope.value) > 0) {
                ret.name = sv_copy(json_at(&scope.value, 0).value.string);
            }
        }
        if (scope.value.type == JSON_TYPE_STRING) {
            ret.scope = sv_split(sv_copy(scope.value.string), SV(",", 1));
            for (size_t ix = 0; ix < ret.scope.size; ++ix) {
                ret.scope.strings[ix] = sv_strip(ret.scope.strings[ix]);
            }
        } else {
            for (size_t ix = 0; ix < json_len(&scope.value); ++ix) {
                JSONValue entry = MUST_OPTIONAL(JSONValue, json_at(&scope.value, ix));
                assert(entry.type == JSON_TYPE_STRING);
                sl_push(&ret.scope, sv_copy(entry.string));
            }
        }
    }
    if (sv_empty(ret.name)) {
        ret.name = sv_copy_cstr("(root)");
    }
    OptionalJSONValue settings = json_get(json, "settings");
    if (!settings.has_value) {
        ERROR(TokenColour, IOError, 0, "'tokenColor' entry must specify settings");
    }
    if (settings.value.type != JSON_TYPE_OBJECT) {
        ERROR(TokenColour, IOError, 0, "'tokenColor' entry settings must be an object");
    }
    ret.colours.fg = TRY_TO(Colour, TokenColour, colour_decode(json_get(&settings.value, "foreground")));
    ret.colours.bg = TRY_TO(Colour, TokenColour, colour_decode(json_get(&settings.value, "background")));
    RETURN(TokenColour, ret);
}

ErrorOrSemanticTokenColour semantic_token_colour_decode(SemanticTokenTypes type, OptionalJSONValue settings)
{
    if (!settings.has_value) {
        ERROR(SemanticTokenColour, IOError, 0, "'semanticTokenColors' entry must specify settings");
    }
    if (settings.value.type != JSON_TYPE_OBJECT) {
        ERROR(SemanticTokenColour, IOError, 0, "'semanticTokenColors' entry settings must be an object");
    }
    SemanticTokenColour ret = { 0 };
    ret.token_type = type;
    ret.colours.fg = TRY_TO(Colour, SemanticTokenColour, colour_decode(json_get(&settings.value, "foreground")));
    ret.colours.bg = TRY_TO(Colour, SemanticTokenColour, colour_decode(json_get(&settings.value, "background")));
    RETURN(SemanticTokenColour, ret);
}

void theme_get_mapping(Theme *theme, TokenKind kind, char const *scope)
{
    OptionalInt index = theme_index_for_scope(theme, sv_from(scope));
    if (index.has_value) {
        trace(EDIT, "Mapping token kind '%.*s' to scope '%s', color %.*s",
            SV_ARG(TokenKind_name(kind)),
            scope,
            SV_ARG(colour_to_rgb(theme->token_colours.elements[index.value].colours.fg)));
        da_append_TokenThemeMapping(&theme->token_mappings, (TokenThemeMapping) { .kind = kind, .theme_index = index.value });
        return;
    }
    trace(EDIT, "Mapping token kind '%.*s' to scope '%s' which is not found",
        SV_ARG(TokenKind_name(kind)), scope);
}

void theme_build_theme_index_mappings(Theme *theme)
{
    theme_get_mapping(theme, TK_COMMENT, "comment");
    theme_get_mapping(theme, TK_KEYWORD, "keyword");
    theme_get_mapping(theme, TK_IDENTIFIER, "identifier");
    theme_get_mapping(theme, TK_NUMBER, "constant.numeric");
    theme_get_mapping(theme, TK_SYMBOL, "punctuation");
    theme_get_mapping(theme, TK_QUOTED_STRING, "string");
    theme_get_mapping(theme, TK_DIRECTIVE, "keyword.control.directive");
    theme_get_mapping(theme, TK_DIRECTIVE_ARG, "string");
}

ErrorOrTheme theme_decode(Theme *theme, JSONValue *json)
{
    JSONValue colors = json_get_default(json, "colors", json_object());
    if (colors.type != JSON_TYPE_OBJECT) {
        ERROR(Theme, ParserError, 0, "'colors' section of theme definition must be a JSON object");
    }
    assert(colors.type == JSON_TYPE_OBJECT);
    theme->editor.bg = TRY_TO(Colour, Theme, colour_decode(json_get(&colors, "editor.background")));
    if (theme->editor.bg.rgba == 0) {
        theme->editor.bg = (Colour) { .rgba = 0x000000FF };
    }
    theme->editor.fg = TRY_TO(Colour, Theme, colour_decode(json_get(&colors, "editor.foreground")));
    if (theme->editor.fg.rgba == 0) {
        theme->editor.fg = (Colour) { .rgba = 0xFFFFFFFF };
    }
    theme->selection.bg = TRY_TO(Colour, Theme, colour_decode(json_get(&colors, "editor.selectionBackground")));
    theme->selection.fg = TRY_TO(Colour, Theme, colour_decode(json_get(&colors, "editor.selectionForeground")));
    if (theme->selection.bg.rgba == 0 && theme->selection.fg.rgba == 0) {
        theme->selection.bg = theme->editor.fg;
        theme->selection.fg = theme->editor.bg;
    }
    if (theme->selection.bg.rgba == 0) {
        theme->selection.bg = theme->editor.bg;
    }
    if (theme->selection.fg.rgba == 0) {
        theme->selection.fg = theme->editor.fg;
    }
    theme->linehighlight.bg = TRY_TO(Colour, Theme, colour_decode(json_get(&colors, "editor.lineHighlightBackground")));
    theme->linehighlight.fg = TRY_TO(Colour, Theme, colour_decode(json_get(&colors, "editor.lineHighlightForeground")));
    if (theme->selection.bg.rgba == 0) {
        theme->selection.bg = theme->editor.bg;
    }
    if (theme->selection.fg.rgba == 0) {
        theme->selection.fg = theme->editor.fg;
    }
    theme->gutter.bg = TRY_TO(Colour, Theme, colour_decode(json_get(&colors, "editorGutter.background")));
    theme->gutter.fg = TRY_TO(Colour, Theme, colour_decode(json_get(&colors, "editor.activeLineNumber.foreground")));
    if (theme->gutter.bg.rgba == 0) {
        theme->gutter.bg = theme->editor.bg;
    }
    if (theme->gutter.fg.rgba == 0) {
        theme->gutter.fg = theme->editor.fg;
    }
    JSONValue token_colors = json_get_default(json, "tokenColors", json_array());
    if (token_colors.type != JSON_TYPE_ARRAY) {
        ERROR(Theme, ParserError, 0, "'tokenColors' section of theme definition must be a JSON array");
    }
    for (size_t ix = 0; ix < json_len(&token_colors); ++ix) {
        JSONValue   c = MUST_OPTIONAL(JSONValue, json_at(&token_colors, ix));
        TokenColour tc = TRY_TO(TokenColour, Theme, token_colour_decode(&c));
        if (tc.colours.bg.rgba == 0) {
            tc.colours.bg = theme->editor.bg;
        }
        if (tc.colours.fg.rgba == 0) {
            tc.colours.fg = theme->editor.fg;
        }
        da_append_TokenColour(&theme->token_colours, tc);
    }

    JSONValue semantic_token_colors = json_get_default(json, "semanticTokenColors", json_object());
    if (semantic_token_colors.type != JSON_TYPE_OBJECT) {
        ERROR(Theme, ParserError, 0, "'semanticTokenColors' section of theme definition must be a JSON object");
    }
    for (size_t ix = 0; ix < json_len(&semantic_token_colors); ++ix) {
        JSONNVPair                *pair = da_element_JSONNVPair(&semantic_token_colors.object, ix);
        OptionalSemanticTokenTypes type = SemanticTokenTypes_decode(OptionalJSONValue_create(json_string(pair->name)));
        if (!type.has_value) {
            continue;
        }
        OptionalJSONValue   settings = OptionalJSONValue_create(pair->value);
        SemanticTokenColour semantic_token_colour = TRY_TO(SemanticTokenColour, Theme, semantic_token_colour_decode(type.value, settings));
        da_append_SemanticTokenColour(&theme->semantic_colours, semantic_token_colour);
    }

    theme_build_theme_index_mappings(theme);
    RETURN(Theme, *theme);
}

ErrorOrTheme theme_load(StringView name)
{
    Theme          ret = { 0 };
    struct passwd *pw = getpwuid(getuid());
    StringBuilder  eddy_fname = sb_createf("%s/.eddy", pw->pw_dir);
    TRY_TO(Int, Theme, fs_assert_dir(eddy_fname.view));
    sb_append_cstr(&eddy_fname, "/themes");
    TRY_TO(Int, Theme, fs_assert_dir(eddy_fname.view));
    StringView theme_fname = sv_printf("%.*s/%.*s.json", SV_ARG(eddy_fname), SV_ARG(name));
    sv_free(eddy_fname.view);
    if (!fs_file_exists(theme_fname)) {
        sv_free(theme_fname);
        theme_fname = sv_printf(EDDY_DATADIR "/themes/%.*s.json", SV_ARG(name));
        if (!fs_file_exists(theme_fname)) {
            ERROR(Theme, IOError, 0, "Theme file '%.*s.json' not found", SV_ARG(name));
        }
    }
    StringView s = TRY_TO(StringView, Theme, read_file_by_name(theme_fname));
    sv_free(theme_fname);
    JSONValue theme = TRY_TO(JSONValue, Theme, json_decode(s));
    sv_free(s);
    TRY(Theme, theme_decode(&ret, &theme));
    json_free(theme);
    RETURN(Theme, ret);
}

OptionalInt theme_index_for_scope(Theme *theme, StringView scope)
{
    int    tc_match = -1;
    size_t matchlen = 0;
    for (int tc_ix = 0; tc_ix < theme->token_colours.size; ++tc_ix) {
        TokenColour *tc = theme->token_colours.elements + tc_ix;
        if (tc->scope.size == 0 && tc_match == -1) {
            assert(matchlen == 0);
            tc_match = tc_ix;
            continue;
        }
        for (size_t scope_ix = 0; scope_ix < tc->scope.size; ++scope_ix) {
            if (sv_startswith(scope, tc->scope.strings[scope_ix]) && tc->scope.strings[scope_ix].length > matchlen) {
                tc_match = tc_ix;
                matchlen = tc->scope.strings[scope_ix].length;
            }
        }
    }
    if (tc_match >= 0) {
        RETURN_VALUE(Int, tc_match);
    }
    RETURN_EMPTY(Int);
}

OptionalColours theme_token_colours(Theme *theme, Token t)
{
    for (size_t ix = 0; ix < theme->token_mappings.size; ++ix) {
        TokenThemeMapping *mapping = theme->token_mappings.elements + ix;
        if (mapping->kind == t.kind) {
            RETURN_VALUE(Colours, theme->token_colours.elements[mapping->theme_index].colours);
        }
    }
    RETURN_EMPTY(Colours);
}

OptionalColours theme_semantic_colours(Theme *theme, int semantic_index)
{
    for (size_t ix = 0; ix < theme->semantic_mappings.size; ++ix) {
        SemanticMapping *mapping = theme->semantic_mappings.elements + ix;
        if (mapping->semantic_index == semantic_index) {
            if (mapping->semantic_theme_index < 0 && mapping->token_theme_index >= 0) {
                RETURN_VALUE(Colours, theme->token_colours.elements[mapping->token_theme_index].colours);
            }
            if (mapping->semantic_theme_index >= 0) {
                RETURN_VALUE(Colours, theme->semantic_colours.elements[mapping->semantic_theme_index].colours);
            }
            RETURN_EMPTY(Colours);
        }
    }
    RETURN_EMPTY(Colours);
}

typedef struct {
    SemanticTokenTypes semantic_type;
    char const        *scope;
} SemanticTypeToScopeMapping;

SemanticTypeToScopeMapping semantic_scope_mapping[] = {
    { SemanticTokenTypesType, "entity.name.type" },
    { SemanticTokenTypesClass, "entity.name.type.class" },
    { SemanticTokenTypesEnum, "entity.name.type.enum" },
    { SemanticTokenTypesParameter, "variable.parameter" },
    { SemanticTokenTypesVariable, "variable.other.readwrite" },
    { SemanticTokenTypesProperty, "variable.other.property" },
    { SemanticTokenTypesEnumMember, "variable.other.enummember" },
    { SemanticTokenTypesFunction, "entity.name.function" },
    { SemanticTokenTypesMacro, "entity.name.function.preprocessor" },
    { SemanticTokenTypesKeyword, "keyword" },
    { SemanticTokenTypesComment, "comment" },
    { SemanticTokenTypesString, "string" },
    { SemanticTokenTypesNumber, "constant.numeric" },
    { SemanticTokenTypesOperator, "keyword.operator" },
};

void theme_map_semantic_type(Theme *theme, int semantic_index, SemanticTokenTypes type)
{
    for (size_t ix = 0; ix < theme->semantic_colours.size; ++ix) {
        SemanticTokenColour *colour = theme->semantic_colours.elements + ix;
        if (colour->token_type == type) {
            trace(LSP, "Mapping SemanticTokenType %d = '%.*s' to theme semantic index %zu", semantic_index, SV_ARG(SemanticTokenTypes_to_string(type)), ix);
            da_append_SemanticMapping(&theme->semantic_mappings, (SemanticMapping) { .semantic_index = semantic_index, .semantic_theme_index = ix, .token_theme_index = -1 });
            return;
        }
    }
    for (size_t ix = 0; ix < sizeof(semantic_scope_mapping) / sizeof(SemanticTypeToScopeMapping); ++ix) {
        if (semantic_scope_mapping[ix].semantic_type == type) {
            OptionalInt theme_ix_maybe = theme_index_for_scope(theme, sv_from(semantic_scope_mapping[ix].scope));
            int         theme_ix = -1;
            if (theme_ix_maybe.has_value) {
                theme_ix = theme_ix_maybe.value;
            }
            trace(LSP, "Mapping SemanticTokenType %d = '%.*s' to scope '%s'", semantic_index, SV_ARG(SemanticTokenTypes_to_string(type)), semantic_scope_mapping[ix].scope);
            da_append_SemanticMapping(&theme->semantic_mappings, (SemanticMapping) { .semantic_index = semantic_index, .semantic_theme_index = -1, .token_theme_index = theme_ix });
            return;
        }
    }
    trace(LSP, "SemanticTokenType %d = '%.*s' not mapped", semantic_index, SV_ARG(SemanticTokenTypes_to_string(type)));
}
