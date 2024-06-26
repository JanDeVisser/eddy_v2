/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __BINDER_H__
#define __BINDER_H__

#include <base/json.h>
#include <base/sv.h>
#include <scribble/engine.h>
#include <scribble/parser.h>
#include <scribble/type.h>

#define BOUNDNODETYPES(S) \
    S(ASSIGNMENT)         \
    S(BINARYEXPRESSION)   \
    S(BLOCK)              \
    S(BOOL)               \
    S(BREAK)              \
    S(CAST)               \
    S(COMPOUND)           \
    S(CONST)              \
    S(CONTINUE)           \
    S(DECIMAL)            \
    S(ENUMERATION)        \
    S(ENUM_VALUE)         \
    S(FOR)                \
    S(FUNCTION)           \
    S(FUNCTION_CALL)      \
    S(FUNCTION_IMPL)      \
    S(NAME)               \
    S(IF)                 \
    S(IMPORT)             \
    S(INTEGER)            \
    S(LOOP)               \
    S(MACRO)              \
    S(MODULE)             \
    S(NATIVE_FUNCTION)    \
    S(PARAMETER)          \
    S(PROGRAM)            \
    S(RETURN)             \
    S(STRING)             \
    S(STRUCT)             \
    S(TERNARYEXPRESSION)  \
    S(TYPE)               \
    S(TYPE_COMPONENT)     \
    S(UNARYEXPRESSION)    \
    S(UNBOUND_NODE)       \
    S(UNBOUND_TYPE)       \
    S(VARIABLE)           \
    S(VARIABLE_DECL)      \
    S(VARIANT)            \
    S(VARIANT_OPTION)     \
    S(WHILE)

typedef enum bound_node_type {
    BNT_OFFSET = 1000,
#undef BOUNDNODETYPE_ENUM
#define BOUNDNODETYPE_ENUM(type) BNT_##type,
    BOUNDNODETYPES(BOUNDNODETYPE_ENUM)
#undef BOUNDNODETYPE_ENUM
        BNT_LAST
} BoundNodeType;

typedef struct bind_error {
    Token              token;
    StringView         message;
    struct bind_error *notes;
    struct bind_error *next;
} BindError;

DA(BindError);
typedef DA_BindError BindErrors;

typedef struct bind_context {
    struct bind_context *parent;
    int                  unbound;
    SyntaxNode          *program;
    bool                 main;
    int                  errors;
    int                  warnings;
    BindErrors           bind_errors;
    socket_t             frontend;
    bool                 debug;
} BindContext;

typedef struct bound_node {
    BoundNodeType        type;
    StringView           name;
    struct bound_node   *next;
    struct bound_node   *prev;
    size_t               index;
    struct bound_node   *parent;
    TypeSpec             typespec;
    TokenLocation        location;
    size_t               token_length;
    struct intermediate *intermediate;
    union {
        struct {
            struct bound_node *variable;
            struct bound_node *expression;
        } assignment;
        struct {
            struct bound_node *lhs;
            struct bound_node *rhs;
            Operator           operator;
        } binary_expr;
        struct {
            struct bound_node *statements;
        } block;
        struct {
            struct bound_node *function;
            struct bound_node *argument;
            bool               discard_result;
        } call;
        struct {
            struct bound_node *expr;
            type_id            cast_to;
        } cast_expr;
        struct {
            struct bound_node *components;
        } compound_def;
        struct {
            struct bound_node *expressions;
        } compound_expr;
        struct bound_node *const_expr;
        struct bound_node *controlled_statement;
        double             decimal_value;
        struct {
            TypeSpec           underlying_type;
            struct bound_node *values;
        } enumeration;
        struct {
            struct bound_node *underlying_value;
        } enum_value;
        struct {
            struct bound_node *variable;
            struct bound_node *range;
            struct bound_node *statement;
        } for_statement;
        struct {
            struct bound_node *parameter;
            struct bound_node *function_impl;
        } function;
        struct {
            struct bound_node *condition;
            struct bound_node *if_true;
            struct bound_node *if_false;
        } if_statement;
        struct {
            struct bound_node *modules;
        } import;
        Integer integer;
        struct {
            struct bound_node *types;
            struct bound_node *imports;
            struct bound_node *modules;
        } program;
        struct {
            struct bound_node *expression;
        } return_stmt;
        struct {
            struct bound_node *condition;
            struct bound_node *if_true;
            struct bound_node *if_false;
        } ternary_expr;
        struct {
            type_id alias_of;
        } type_alias;
        struct {
            struct bound_node *operand;
            Operator           operator;
        } unary_expr;
        struct {
            type_id            type;
            struct bound_node *decl;
            struct bound_node *subscript;
        } variable;
        struct {
            bool is_const;
        } variable_decl;
        struct {
            TypeSpec           underlying_type;
            struct bound_node *options;
        } variant_def;
        struct {
            struct bound_node *underlying_value;
            TypeSpec           payload_type;
        } variant_option;
        struct {
            struct bound_node *condition;
            struct bound_node *statement;
        } while_statement;
        SyntaxNode *unbound_node;
    };
} BoundNode;

extern char const *BoundNodeType_name(BoundNodeType type);
extern JSONValue   bound_node_to_json(BoundNode *node);
extern void        bound_node_chain_to_json(BoundNode *node, JSONValue *array);
extern BoundNode  *bind_program(BackendConnection *conn, JSONValue config, SyntaxNode *program);
extern BoundNode  *bind_format(BoundNode *node, void *v_ctx);

#endif /* __BINDER_H__ */
