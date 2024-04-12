/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __PARSER_H__
#define __PARSER_H__

#include <base/json.h>
#include <base/lexer.h>
#include <base/sv.h>
#include <base/token.h>
#include <scribble/engine.h>
#include <scribble/model/error.h>
#include <scribble/model/syntaxnode.h>

typedef struct operator_mapping {
    Operator  operator;
    bool      binary;
    TokenKind token_kind;
    int       token_code;
    char      closed_by;
    int       precedence;
} OperatorMapping;

typedef struct parser_context {
    socket_t       frontend;
    bool           debug;
    Lexer         *lexer;
    SyntaxNode    *program;
    ScribbleErrors errors;
    StringView     source_name;
} ParserContext;

extern size_t        next_index();
extern SyntaxNode   *syntax_node_make(ParserContext *ctx, SyntaxNodeType type, StringView name, Token token);
extern ParserContext parse(BackendConnection *conn, JSONValue config);

#endif /* __PARSER_H__ */
