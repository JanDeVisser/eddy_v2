/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <token.h>

char const *TokenKind_name(TokenKind kind)
{
    switch (kind) {
#undef TOKENKIND_ENUM
#define TOKENKIND_ENUM(kind) \
    case TK_##kind:          \
        return #kind;
        TOKENKINDS(TOKENKIND_ENUM)
#undef TOKENKIND_ENUM
    default:
        UNREACHABLE();
    }
}

TokenKind TokenKind_from_string(StringView kind)
{
#undef TOKENKIND
#define TOKENKIND(K)                      \
    if (sv_eq_ignore_case_cstr(kind, #K)) \
        return TK_##K;
    TOKENKINDS(TOKENKIND)
#undef TOKENKIND
    fatal("Unrecognized token kind '%.*s'", SV_ARG(kind));
}

char const *TokenCode_name(int code)
{
    switch ((TokenCode) code) {
#undef TOKENCODE_ENUM
#define TOKENCODE_ENUM(code) \
    case code:               \
        return #code;
        TOKENCODES(TOKENCODE_ENUM)
#undef TOKENCODE_ENUM
    default:
        return "Unknown";
    }
}

JSONValue token_to_json(Token token)
{
    JSONValue ret = json_object();
    json_set_string(&ret, "text", token.text);
    json_set_cstr(&ret, "kind", TokenKind_name(token.kind));
    json_set_int(&ret, "code", token.code);
    json_set(&ret, "location", json_int(token.location));
    return ret;
}

Token token_from_json(JSONValue token)
{
    assert(token.type == JSON_TYPE_OBJECT);
    return (Token) {
        .text = json_get_string(&token, "text", sv_null()),
        .kind = TokenKind_from_string(json_get_string(&token, "kind", sv_from("UNKNOWN"))),
        .code = json_get_int(&token, "code", TC_NONE),
        .location = json_get_int(&token, "location", 0),
    };
}

Token token_merge(Token t1, Token t2, TokenKind kind, int code)
{
    Token ret = { 0 };
    if (t1.text.ptr < t2.text.ptr) {
        ret = t1;
        ret.text.length = (t2.text.ptr - t1.text.ptr) + t2.text.length;
    } else {
        ret = t2;
        ret.text.length = (t1.text.ptr - t2.text.ptr) + t1.text.length;
    }
    ret.kind = kind;
    ret.code = code;
    return ret;
}
