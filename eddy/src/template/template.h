/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef TEMPLATE_H
#define TEMPLATE_H

#include <base/error_or.h>
#include <base/json.h>
#include <base/optional.h>
#include <base/sv.h>

#define TPLTOKENTYPES(S) \
    S(Unknown)           \
    S(EndOfText)         \
    S(Whitespace)        \
    S(Comment)           \
    S(Symbol)            \
    S(Identifier)        \
    S(Number)            \
    S(Keyword)           \
    S(Operator)          \
    S(String)            \
    S(True)              \
    S(False)             \
    S(Null)

typedef enum {
#undef S
#define S(T) TTT##T,
    TPLTOKENTYPES(S)
#undef S
} TPLTokenType;

#define TPLKEYWORDS(S)            \
    S(Call, "call", NULL)         \
    S(Case, "case", NULL)         \
    S(Close, ";", NULL)           \
    S(Else, "else", NULL)         \
    S(CloseBlock, "@", "@;")      \
    S(For, "for", NULL)           \
    S(If, "if", NULL)             \
    S(Macro, "macro", NULL)       \
    S(Switch, "switch", NULL)     \
    S(StartExpression, "=", NULL) \
    S(Set, "set", NULL)           \
    S(Text, "\"", "'")

typedef enum {
#undef S
#define S(T, STR, ALT) TKW##T,
    TPLKEYWORDS(S)
#undef S
        TKWCount,
} TplKeyword;

ERROR_OR(TplKeyword);
DA_WITH_NAME(TplKeyword, TplKeywords);

#define TPLOPTOKENS(S)                                               \
    S(Asterisk, "*", BTOMultiply, 12, InvalidOperator, -1)           \
    S(Equals, "==", BTOEquals, 8, InvalidOperator, -1)               \
    S(ExclamationPoint, "!", InvalidOperator, -1, UTOInvert, 14)     \
    S(Greater, ">", BTOGreater, 9, InvalidOperator, -1)              \
    S(GreaterEquals, ">=", BTOGreaterEquals, 9, InvalidOperator, -1) \
    S(Less, "<", BTOLess, 9, InvalidOperator, -1)                    \
    S(LessEquals, "<=", BTOLessEquals, 9, InvalidOperator, -1)       \
    S(Minus, "-", BTOSubtract, 11, UTONegate, 14)                    \
    S(OpenCurly, "{", InvalidOperator, -1, UTODereference, 15)       \
    S(OpenParen, "(", BTOCall, 15, UTOParenthesize, 16)              \
    S(Percent, "%", BTOModulo, 12, InvalidOperator, -1)              \
    S(Period, ".", BTOSubscript, 15, InvalidOperator, -1)            \
    S(Plus, "+", BTOAdd, 11, UTOIdentity, 14)                        \
    S(Slash, "/", BTODivide, 12, InvalidOperator, -1)                \
    S(Unequal, "!=", BTONotEquals, 8, InvalidOperator, -1)

typedef enum {
#undef S
#define S(TOKEN, STR, BINOP, BINPREC, UNOP, UNPREC) TO##TOKEN,
    TPLOPTOKENS(S)
#undef S
        TOCount,
} TplOpToken;

OPTIONAL(TplOpToken)
ERROR_OR(TplOpToken)
ERROR_OR(OptionalTplOpToken)

typedef struct {
    TPLTokenType type;
    TextPosition position;
    StringView   raw_text;
    union {
        StringRef  text;
        int        ch;
        TplOpToken op;
        TplKeyword keyword;
    };
} TplToken;

OPTIONAL(TplToken)
ERROR_OR(TplToken)
ERROR_OR(OptionalTplToken)

#define TEMPLATENODEKINDS(S) \
    S(Text)                  \
    S(Expr)                  \
    S(ForLoop)               \
    S(IfStatement)           \
    S(MacroCall)             \
    S(MacroDef)              \
    S(SetVariable)           \
    S(SwitchStatement)

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
    UTOParenthesize,
    UTODereference,
} TplOperator;

ERROR_OR(TplOperator);
DA_STRUCT_WITH_NAME(TemplateExpression, struct template_expression *, TemplateExpressions);

typedef struct {
    TplOpToken  token;
    char const *string;
    TplOperator binary_op;
    int         binary_precedence;
    TplOperator unary_op;
    int         unary_precedence;
} TplOperatorMapping;

OPTIONAL(TplOperatorMapping)
ERROR_OR(TplOperatorMapping)
ERROR_OR(OptionalTplOperatorMapping)

typedef struct {
    TplKeyword  keyword;
    char const *string;
    char const *alt_string;
} TplKeywordMapping;

OPTIONAL(TplKeywordMapping)
ERROR_OR(TplKeywordMapping)
ERROR_OR(OptionalTplKeywordMapping)

typedef struct template_expression {
    TemplateExpressionType type;
    union {
        StringView raw_text;
        StringRef  text;
        struct {
            struct template_expression *lhs;
            TplOperator                 op;
            struct template_expression *rhs;
        } binary;
        struct {
            TplOperator                 op;
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

DA_STRUCT_WITH_NAME(TemplateNode, struct template_node *, TemplateNodes);

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
            TemplateExpression *condition;
            StringView          macro;
        } for_statement;
        struct {
            TemplateExpression   *condition;
            struct template_node *true_branch;
            struct template_node *false_branch;
        } if_statement;
        struct {
            TemplateExpression *condition;
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
        struct {
            TemplateExpression   *expr;
            struct template_node *cases;
        } switch_statement;
        struct {
            bool literal;
        } text_node;
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
    TplToken       token;
    StringScanner  ss;
    TemplateNode **current;
} TemplateParserContext;

extern char const                       *TplKeyword_name(TplKeyword keyword);
extern char const                       *TemplateNodeKind_name(TemplateNodeKind kind);
extern char const                       *TemplateExpressionType_name(TemplateExpressionType type);
extern char const                       *TplOperator_name(TplOperator op);
extern char const                       *TplTokenType_name(TPLTokenType type);
extern char const                       *TplOpToken_name(TplOpToken token);
extern StringView                        template_token_to_string(TemplateParserContext *ctx, TplToken token);
extern OptionalTplOperatorMapping        template_operator_mapping(TplOpToken token);
extern void                              template_lexer_consume(TemplateParserContext *ctx);
extern ErrorOrTplToken                   template_lexer_peek(TemplateParserContext *ctx);
extern ErrorOrTplToken                   template_lexer_next(TemplateParserContext *ctx);
extern ErrorOrOptionalTplToken           template_lexer_allow_type(TemplateParserContext *ctx, TPLTokenType type);
extern ErrorOrTplToken                   template_lexer_require_type(TemplateParserContext *ctx, TPLTokenType type);
extern ErrorOrOptionalStringView         template_lexer_allow_identifier(TemplateParserContext *ctx);
extern ErrorOrBool                       template_lexer_allow_sv(TemplateParserContext *ctx, StringView string);
extern ErrorOrStringView                 template_lexer_require_identifier(TemplateParserContext *ctx);
extern ErrorOrBool                       template_lexer_allow_symbol(TemplateParserContext *ctx, int symbol);
extern ErrorOrBool                       template_lexer_require_symbol(TemplateParserContext *ctx, int symbol);
extern ErrorOrTplToken                   template_lexer_require_one_of(TemplateParserContext *ctx, char *symbols);
extern ErrorOrBool                       template_lexer_allow_keyword(TemplateParserContext *ctx, TplKeyword keyword);
extern ErrorOrBool                       template_lexer_require_keyword(TemplateParserContext *ctx, TplKeyword keyword);
extern StringView                        template_expression_to_string(Template tpl, TemplateExpression *expr);
extern JSONValue                         template_expression_serialize(Template tpl, TemplateExpression *expr);
extern ErrorOrOptionalTplOperatorMapping template_lexer_operator(TemplateParserContext *ctx);
extern ErrorOrTemplateExpression         template_ctx_parse_expression(TemplateParserContext *ctx);
extern ErrorOrTplKeyword                 template_ctx_parse_nodes(TemplateParserContext *ctx, ...);
extern JSONValue                         template_node_serialize(Template tpl, TemplateNode *node);
extern ErrorOrTemplate                   template_parse(StringView template);
extern ErrorOrStringView                 template_render(Template template, JSONValue context);
extern TemplateNode                     *template_find_macro(Template template, StringView name);
extern ErrorOrStringView                 render_template(StringView template_text, JSONValue context);
extern ErrorOrSize                       render_template_file(StringView template_filename, JSONValue context, StringView output_filename);

#define template_ctx_parse(CTX, ...) template_ctx_parse_nodes((CTX) __VA_OPT__(, ) __VA_ARGS__, TKWCount)

#endif /* TEMPLATE_H */
