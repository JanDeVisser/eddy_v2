/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef TEMPLATE_H
#define TEMPLATE_H

#include <error_or.h>
#include <optional.h>

typedef enum {
    TNKText,
    TNKExpr,
    TNKForLoop,
    TNKIfStatement,
    TNKMacroCall,
    TNKMacroDef,
    TNKSetVariable,
} TemplateNodeKind;

typedef enum {
    UTOIdentity,
    UTONegate,
    UTOInvert,
} UnaryTemplateOperator;

typedef enum {
    TETVariableReference,
    TETBinaryExpression,
    TETUnaryExpression,
    TETNull,
    TETNumber,
    TETBoolean,
    TETString,
} TemplateExpressionType;

typedef enum {
    BTOInvalid,
    BTOAdd,
    BTOSubtract,
    BTOMultiply,
    BTODivide,
    BTOModulo,
    BTOEquals,
    BTONotEquals,
    BTOGreater,
    BTOGreaterEquals,
    BTOLess,
    BTOLessEquals,
    BTOCount,
} BinaryTemplateOperator;

typedef struct template_expression {
    TemplateExpressionType type;
    union {
        StringRef text;
        struct {
            struct template_expression *lhs;
            BinaryTemplateOperator      op;
            struct template_expression *rhs;
        } binary;
        struct {
            UnaryTemplateOperator       op;
            struct template_expression *operand;
        } unary;
        int64_t number;
        bool    boolean;
    };
} TemplateExpression;

ERROR_OR_ALIAS(TemplateExpression, TemplateExpression *)
DA_STRUCT_WITH_NAME(TemplateExpression, TemplateExpression *, TemplateExpressions);

PAIR_WITH_NAME(StringRef, JSONType, Parameter);

typedef struct template_node {
    TemplateNodeKind kind;
    union {
        StringRef           text;
        TemplateExpression *expr;
        struct {
            StringRef             variable;
            StringRef             variable2;
            TemplateExpression   *range;
            struct template_node *contents;
        } for_statement;
        struct {
            TemplateExpression   *condition;
            struct template_node *true_branch;
            struct template_node *false_branch;
        } if_statement;
        struct {
            StringRef             macro;
            TemplateExpressions   arguments;
            struct template_node *contents;
        } macro_call;
        struct {
            StringRef             name;
            Parameters            parameters;
            struct template_node *contents;
        } macro_def;
        struct {
            StringRef           variable;
            TemplateExpression *value;
        } set_statement;
    };
    struct template_node *next;
} TemplateNode;

ERROR_OR_ALIAS(TemplateNode, TemplateNode *)
PAIR_WITH_NAME(StringRef, TemplateNode *, Macro);

typedef struct {
    StringBuilder sb;
    StringView    text;
    TemplateNode *node;
    Macros        macros;
} Template;

ERROR_OR(Template);

extern ErrorOrTemplate   template_parse(StringView template);
extern ErrorOrStringView template_render(Template template, JSONValue context);
extern ErrorOrStringView render_template(StringView template_text, JSONValue context);

#endif /* TEMPLATE_H */
