/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef TEMPLATE_H
#define TEMPLATE_H

#include <error_or.h>
#include <json.h>
#include <optional.h>
#include <sv.h>

#define TEMPLATENODEKINDS(S) \
    S(Text)                  \
    S(Expr)                  \
    S(ForLoop)               \
    S(IfStatement)           \
    S(MacroCall)             \
    S(MacroDef)              \
    S(SetVariable)

typedef enum {
#undef KIND
#define KIND(K) TNK##K,
    TEMPLATENODEKINDS(KIND)
#undef KIND
} TemplateNodeKind;

#define TEMPLATEEXPRESSIONTYPES(S) \
    S(Identifier)                  \
    S(Dereference)                 \
    S(BinaryExpression)            \
    S(UnaryExpression)             \
    S(FunctionCall)                \
    S(Null)                        \
    S(Number)                      \
    S(Boolean)                     \
    S(String)

typedef enum {
#undef TYPE
#define TYPE(T) TET##T,
    TEMPLATEEXPRESSIONTYPES(TYPE)
#undef TYPE
} TemplateExpressionType;

typedef enum {
    InvalidOperator = 0,
    BTOCall,
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
    BTOSubscript,
    BTODereference,
    UTOIdentity,
    UTONegate,
    UTOInvert,
    UTODereference,
} TemplateOperator;

ERROR_OR(TemplateOperator);
DA_STRUCT_WITH_NAME(TemplateExpression, struct template_expression *, TemplateExpressions);

typedef struct template_expression {
    TemplateExpressionType type;
    union {
        StringRef text;
        struct {
            struct template_expression *lhs;
            TemplateOperator            op;
            struct template_expression *rhs;
        } binary;
        struct {
            TemplateOperator            op;
            struct template_expression *operand;
        } unary;
        struct {
            struct template_expression *function;
            TemplateExpressions         arguments;
        } function_call;
        struct template_expression *dereference;
        int32_t                     number;
        bool                        boolean;
    };
} TemplateExpression;

ERROR_OR_ALIAS(TemplateExpression, TemplateExpression *)

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

extern char const       *TemplateNodeKind_name(TemplateNodeKind kind);
extern char const       *TemplateExpressionType_name(TemplateExpressionType type);
extern char const       *TemplateOperator_name(TemplateOperator op);
extern JSONValue         template_expression_serialize(Template tpl, TemplateExpression *expr);
extern JSONValue         template_node_serialize(Template tpl, TemplateNode *node);
extern ErrorOrTemplate   template_parse(StringView template);
extern ErrorOrStringView template_render(Template template, JSONValue context);
extern TemplateNode     *template_find_macro(Template template, StringRef name);
extern ErrorOrStringView render_template(StringView template_text, JSONValue context);

#endif /* TEMPLATE_H */
