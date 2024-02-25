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
} TemplateNodeKind;

typedef enum {
    BTOInvalid,
    BTOAdd,
    BTOSubtract,
    BTOMultiply,
    BTODivide,
    BTOModulo,
    BTOCount,
} BinaryTemplateOperator;

typedef enum {
    UTOIdentity,
    UTONegate,
    UTOInvert,
} UnaryTemplateOperator;

typedef enum {
    TETVariableReference,
    TETBinaryExpression,
    TETUnaryExpression,
    TETNumber,
    TETBoolean,
    TETString,
} TemplateExpressionType;

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

typedef struct template_node {
    TemplateNodeKind kind;
    union {
        StringRef           text;
        TemplateExpression *expr;
        struct {
            StringRef             variable;
            TemplateExpression   *range;
            struct template_node *contents;
        } for_statement;
    };
    struct template_node *next;
} TemplateNode;

ERROR_OR_ALIAS(TemplateNode, TemplateNode *)

typedef struct {
    StringBuilder sb;
    StringView    text;
    TemplateNode *node;
} Template;

ERROR_OR(Template);

extern ErrorOrTemplate   template_parse(StringView template);
extern ErrorOrStringView template_render(Template template, JSONValue context);
extern ErrorOrStringView render_template(StringView template_text, JSONValue context);

#endif /* TEMPLATE_H */
