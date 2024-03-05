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

#define TEMPLATEEXPRESSIONTOKENTYPES(S) \
    S(Unknown)                          \
    S(EndOfText)                        \
    S(EndOfExpression)                  \
    S(EndOfStatement)                   \
    S(EndOfStatementBlock)              \
    S(StartOfStatement)                 \
    S(StartOfExpression)                \
    S(Whitespace)                       \
    S(Comment)                          \
    S(Symbol)                           \
    S(Identifier)                       \
    S(Number)                           \
    S(Operator)                         \
    S(String)                           \
    S(True)                             \
    S(False)                            \
    S(Null)

typedef enum {
#undef S
#define S(T) TETT##T,
    TEMPLATEEXPRESSIONTOKENTYPES(S)
#undef S
} TemplateExpressionTokenType;
#define TEMPLATEOPERATORTOKENS(S)                                    \
    S(Asterisk, "*", BTOMultiply, 12, InvalidOperator, -1)           \
    S(Equals, "==", BTOEquals, 8, InvalidOperator, -1)               \
    S(ExclamationPoint, "!", InvalidOperator, -1, UTOInvert, 14)     \
    S(Greater, ">", BTOGreater, 9, InvalidOperator, -1)              \
    S(GreaterEquals, ">=", BTOGreaterEquals, 9, InvalidOperator, -1) \
    S(Less, "<", BTOLess, 9, InvalidOperator, -1)                    \
    S(LessEquals, "<=", BTOLessEquals, 9, InvalidOperator, -1)       \
    S(Minus, "-", BTOSubtract, 11, UTONegate, 14)                    \
    S(OpenCurly, "{", InvalidOperator, -1, UTODereference, 15)       \
    S(OpenParen, "(", BTOCall, 15, InvalidOperator, -1)              \
    S(Percent, "%", BTOModulo, 12, InvalidOperator, -1)              \
    S(Period, ".", BTOSubscript, 15, InvalidOperator, -1)            \
    S(Plus, "+", BTOAdd, 11, UTOIdentity, 14)                        \
    S(Slash, "/", BTODivide, 12, InvalidOperator, -1)                \
    S(Unequal, "!=", BTONotEquals, 8, InvalidOperator, -1)

typedef enum {
#undef S
#define S(TOKEN, STR, BINOP, BINPREC, UNOP, UNPREC) TOT##TOKEN,
    TEMPLATEOPERATORTOKENS(S)
#undef S
        TOTCount,
} TemplateOperatorToken;

OPTIONAL(TemplateOperatorToken)
ERROR_OR(TemplateOperatorToken)
ERROR_OR(OptionalTemplateOperatorToken)

typedef struct {
    TemplateExpressionTokenType type;
    StringView                  raw_text;
    union {
        StringRef             text;
        int                   ch;
        TemplateOperatorToken op;
    };
} TemplateExpressionToken;

OPTIONAL(TemplateExpressionToken)
ERROR_OR(TemplateExpressionToken)
ERROR_OR(OptionalTemplateExpressionToken)

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

typedef struct {
    TemplateOperatorToken token;
    char const           *string;
    TemplateOperator      binary_op;
    int                   binary_precedence;
    TemplateOperator      unary_op;
    int                   unary_precedence;
} TemplateOperatorMapping;

OPTIONAL(TemplateOperatorMapping)
ERROR_OR(TemplateOperatorMapping)
ERROR_OR(OptionalTemplateOperatorMapping)

typedef struct template_expression {
    TemplateExpressionType type;
    union {
        StringView raw_text;
        StringRef  text;
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

PAIR_WITH_NAME(StringView, JSONType, Parameter);

typedef struct template_node {
    TemplateNodeKind      kind;
    struct template_node *contents;
    union {
        StringRef           text;
        TemplateExpression *expr;
        struct {
            StringView          variable;
            StringView          variable2;
            TemplateExpression *range;
            StringView          macro;
        } for_statement;
        struct {
            TemplateExpression   *condition;
            struct template_node *true_branch;
            struct template_node *false_branch;
        } if_statement;
        struct {
            StringView          macro;
            TemplateExpressions arguments;
        } macro_call;
        struct {
            StringView name;
            Parameters parameters;
        } macro_def;
        struct {
            StringView          variable;
            TemplateExpression *value;
        } set_statement;
    };
    struct template_node *next;
} TemplateNode;

ERROR_OR_ALIAS(TemplateNode, TemplateNode *)
PAIR_WITH_NAME(StringView, TemplateNode *, Macro);

typedef struct {
    StringBuilder sb;
    StringView    text;
    TemplateNode *node;
    Macros        macros;
} Template;

ERROR_OR(Template);

typedef struct {
    union {
        Template template;
        struct {
            StringBuilder sb;
            StringView    text;
            TemplateNode *node;
            Macros        macros;
        };
    };
    TemplateExpressionToken token;
    StringScanner           ss;
    TemplateNode          **current;
} TemplateParserContext;

extern char const                            *TemplateNodeKind_name(TemplateNodeKind kind);
extern char const                            *TemplateExpressionType_name(TemplateExpressionType type);
extern char const                            *TemplateOperator_name(TemplateOperator op);
extern char const                            *TemplateExpressionTokenType_name(TemplateExpressionTokenType type);
extern char const                            *TemplateOperatorToken_name(TemplateOperatorToken token);
extern StringView                             template_token_to_string(TemplateParserContext *ctx, TemplateExpressionToken token);
extern OptionalTemplateOperatorMapping        template_operator_mapping(TemplateOperatorToken token);
extern void                                   template_lexer_consume(TemplateParserContext *ctx);
extern ErrorOrTemplateExpressionToken         template_lexer_peek(TemplateParserContext *ctx);
extern ErrorOrTemplateExpressionToken         template_lexer_next(TemplateParserContext *ctx);
extern ErrorOrOptionalTemplateExpressionToken template_lexer_allow_type(TemplateParserContext *ctx, TemplateExpressionTokenType type);
extern ErrorOrTemplateExpressionToken         template_lexer_require_type(TemplateParserContext *ctx, TemplateExpressionTokenType type);
extern ErrorOrOptionalStringView              template_lexer_allow_identifier(TemplateParserContext *ctx);
extern ErrorOrBool                            template_lexer_allow_sv(TemplateParserContext *ctx, StringView string);
extern ErrorOrStringView                      template_lexer_require_identifier(TemplateParserContext *ctx);
extern ErrorOrBool                            template_lexer_allow_symbol(TemplateParserContext *ctx, int symbol);
extern ErrorOrBool                            template_lexer_require_symbol(TemplateParserContext *ctx, int symbol);
extern ErrorOrTemplateExpressionToken         template_lexer_require_one_of(TemplateParserContext *ctx, char *symbols);
extern JSONValue                              template_expression_serialize(Template tpl, TemplateExpression *expr);
extern ErrorOrOptionalTemplateOperatorMapping template_lexer_operator(TemplateParserContext *ctx);
extern ErrorOrTemplateExpression              template_ctx_parse_expression(TemplateParserContext *ctx);
extern ErrorOrStringView                      template_ctx_parse_nodes(TemplateParserContext *ctx, ...);
extern JSONValue                              template_node_serialize(Template tpl, TemplateNode *node);
extern ErrorOrTemplate                        template_parse(StringView template);
extern ErrorOrStringView                      template_render(Template template, JSONValue context);
extern TemplateNode                          *template_find_macro(Template template, StringView name);
extern ErrorOrStringView                      render_template(StringView template_text, JSONValue context);

#define template_ctx_parse(CTX, ...) template_ctx_parse_nodes((CTX), __VA_ARGS__ __VA_OPT__(, ) NULL)

#endif /* TEMPLATE_H */
