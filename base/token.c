/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <token.h>

StringView TokenKind_name(TokenKind kind)
{
    switch (kind) {
#undef S
#define S(kind)     \
    case TK_##kind: \
        return (StringView) { #kind, strlen(#kind) };
        TOKENKINDS(S)
#undef S
    default:
        UNREACHABLE();
    }
}

ErrorOrTokenKind TokenKind_from_string(StringView kind)
{
#undef S
#define S(K)                              \
    if (sv_eq_ignore_case_cstr(kind, #K)) \
        RETURN(TokenKind, TK_##K);
    TOKENKINDS(S)
#undef S
    ERROR(TokenKind, ValueError, 0, "Invalid token kind '%.*s'", SV_ARG(kind));
}

StringView QuoteType_name(QuoteType type)
{
    switch (type) {
#undef S
#define S(T, Q)                        \
    case QT##T: {                      \
        char q = Q;                    \
        return (StringView) { &q, 1 }; \
    }
        QUOTETYPES(S)
#undef S
    default:
        UNREACHABLE();
    }
}

ErrorOrQuoteType QuoteType_from_string(StringView type)
{
    if (type.length != 1) {
        ERROR(QuoteType, ValueError, 0, "Invalid quote type '%.*s'", SV_ARG(type));
    }
#undef S
#define S(T, Q)           \
    if (type.ptr[0] == Q) \
        RETURN(QuoteType, QT##T);
    QUOTETYPES(S)
#undef S
    ERROR(QuoteType, ValueError, 0, "Invalid quote type '%.*s'", SV_ARG(type));
}

StringView CommentType_name(CommentType type)
{
    switch (type) {
#undef COMMENTTYPE_ENUM
#define S(T)                                    \
    case CT##T:                                 \
        return (StringView) { #T, strlen(#T) }; \
        COMMENTTYPES(S)
#undef S
    default:
        UNREACHABLE();
    }
}

ErrorOrCommentType CommentType_from_string(StringView type)
{
#undef S
#define S(T)           \
    if (sv_eq_ignore_case_cstr(type, #T)) \
        RETURN(CommentType, CT##T);
    COMMENTTYPES(S)
#undef S
    ERROR(CommentType, ValueError, 0, "Invalid comment type '%.*s'", SV_ARG(type));
}

StringView NumberType_name(NumberType type)
{
    switch (type) {
#undef S
#define S(T)                                    \
    case NT##T:                                 \
        return (StringView) { #T, strlen(#T) };
        NUMBERTYPES(S)
#undef S
    default:
        UNREACHABLE();
    }
}

ErrorOrNumberType NumberType_from_string(StringView type)
{
#undef S
#define S(T)           \
    if (sv_eq_ignore_case_cstr(type, #T)) \
        RETURN(NumberType, NT##T);
    NUMBERTYPES(S)
#undef S
    ERROR(NumberType, ValueError, 0, "Invalid number type '%.*s'", SV_ARG(type));
}

JSONValue location_to_json(TokenLocation location)
{
    JSONValue ret = json_object();
    json_set_string(&ret, "file", location.file);
    json_set_int(&ret, "line", (int) location.line);
    json_set_int(&ret, "column", (int) location.column);
    return ret;
}

ErrorOrTokenLocation location_from_json(JSONValue location)
{
    if (location.type != JSON_TYPE_OBJECT) {
        ERROR(TokenLocation , ValueError, 0, "Cannot decode JSON type '%s' to location", JSONType_name(location.type));
    }
    TokenLocation ret = {
        .file = json_get_string(&location, "file", sv_null()),
        .line = json_get_int(&location, "line", 0),
        .column = json_get_int(&location, "column", 0),
    };
    RETURN(TokenLocation, ret);
}

JSONValue token_to_json(Token token)
{
    JSONValue ret = json_object();
    json_set_string(&ret, "text", token.text);
    json_set_string(&ret, "kind", TokenKind_name(token.kind));
    json_set(&ret, "location", location_to_json(token.location));
    switch (token.kind) {
    case TK_NUMBER:
        json_set_string(&ret, "number_type", NumberType_name(token.number_type));
        break;
    case TK_QUOTED_STRING: {
        JSONValue quotes = json_object();
        json_set_string(&quotes, "quote_type", QuoteType_name(token.quoted_string.quote_type));
        json_set(&quotes, "triple", json_bool(token.quoted_string.triple));
        json_set(&quotes, "terminated", json_bool(token.quoted_string.terminated));
        json_set(&ret, "quoted_string", quotes);
    } break;
    case TK_COMMENT: {
        JSONValue  comment = json_object();
        json_set_string(&comment, "comment_type", CommentType_name(token.comment.comment_type));
        json_set(&comment, "terminated", json_bool(token.comment.terminated));
        json_set(&ret, "comment", comment);
    } break;
    case TK_KEYWORD:
        json_set_int(&ret, "code", token.keyword_code);
        break;
    case TK_DIRECTIVE:
        json_set_int(&ret, "directive", token.directive);
        break;
    case TK_SYMBOL:
        json_set_int(&ret, "symbol", token.symbol);
        break;
    default:
        // Nothing
        break;
    }
    return ret;
}

ErrorOrToken token_from_json(JSONValue token)
{
    if (token.type != JSON_TYPE_OBJECT) {
        ERROR(Token, ValueError, 0, "Cannot decode JSON type '%s' to token", JSONType_name(token.type));
    }
    StringView kind_string = TRY_TO(StringView, Token, json_must_string(&token, "kind"));
    TokenKind kind = TRY_TO(TokenKind, Token, TokenKind_from_string(kind_string));
    Token ret = {
        .text = json_get_string(&token, "text", sv_null()),
        .kind = kind,
    };
    ret.location = TRY_TO(TokenLocation, Token, location_from_json(json_get_default(&token, "location", json_object())));
    switch (kind) {
    case TK_NUMBER: {
        StringView type = TRY_TO(StringView, Token, json_must_string(&token, "number_type"));
        ret.number_type = TRY_TO(NumberType, Token, NumberType_from_string(type));
    } break;
    case TK_QUOTED_STRING: {
        JSONValue sub = json_get_default(&token, "quoted_string", json_object());
        StringView type = TRY_TO(StringView, Token, json_must_string(&sub, "quote_type"));
        ret.quoted_string.quote_type = TRY_TO(QuoteType, Token, QuoteType_from_string(type));
        ret.quoted_string.triple = json_get_bool(&sub, "triple", true);
        ret.quoted_string.terminated = json_get_bool(&sub, "terminated", true);
    } break;
    case TK_COMMENT: {
        JSONValue sub = json_get_default(&token, "comment", json_object());
        StringView type = TRY_TO(StringView, Token, json_must_string(&sub, "comment_type"));
        ret.comment.comment_type = TRY_TO(CommentType, Token, CommentType_from_string(type));
        ret.comment.terminated = json_get_bool(&sub, "terminated", true);
    } break;
    case TK_KEYWORD:
        ret.keyword_code = json_get_int(&token, "code", 0);
        break;
    case TK_DIRECTIVE:
        ret.directive = json_get_int(&token, "directive", 0);
        break;
    case TK_SYMBOL:
        ret.symbol = json_get_int(&token, "symbol", 0);
        break;
    default:
        // Nothing
        break;
    }
    RETURN(Token, ret);
}
