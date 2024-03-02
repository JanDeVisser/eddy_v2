/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <ctype.h>

#include <template.h>

typedef struct template_render_scope {
    JSONValue                     scope;
    struct template_render_scope *up;
} TemplateRenderScope;

typedef struct {
    union {
        Template template;
        struct {
            StringBuilder sb;
            StringView    text;
            TemplateNode *node;
        };
    };
    StringBuilder        output;
    TemplateRenderScope *scope;
} TemplateRenderContext;

typedef ErrorOrJSONValue (*TemplateFunction)(JSONValue);

typedef struct {
    char const      *name;
    TemplateFunction function;
} TemplateFunctionDescription;

static ErrorOrInt       render_node(TemplateRenderContext *ctx, TemplateNode *node);
static ErrorOrJSONValue evaluate_unary_expression(TemplateRenderContext *ctx, TemplateExpression *expr);
static ErrorOrJSONValue evaluate_binary_expression(TemplateRenderContext *ctx, TemplateExpression *expr);
static ErrorOrJSONValue evaluate_function_call(TemplateRenderContext *ctx, TemplateExpression *expr);
static ErrorOrJSONValue evaluate_identifier(TemplateRenderContext *ctx, TemplateExpression *expr);
static ErrorOrJSONValue evaluate_expression(TemplateRenderContext *ctx, TemplateExpression *expr);
static ErrorOrInt       render_macro(TemplateRenderContext *ctx, TemplateNode *node);
static ErrorOrInt       render_expr_node(TemplateRenderContext *ctx, TemplateNode *node);
static ErrorOrInt       render_for_loop(TemplateRenderContext *ctx, TemplateNode *node);
static ErrorOrInt       render_if_statement(TemplateRenderContext *ctx, TemplateNode *node);
static ErrorOrJSONValue template_function_toupper(JSONValue args);

TemplateFunctionDescription s_functions[] = {
    { "toupper", template_function_toupper },
};

ErrorOrJSONValue evaluate_unary_expression(TemplateRenderContext *ctx, TemplateExpression *expr)
{
    JSONValue value = TRY(JSONValue, evaluate_expression(ctx, expr->unary.operand));
    switch (expr->unary.op) {
    case UTODereference:
    case UTOIdentity:
        RETURN(JSONValue, value);
    case UTOInvert: {
        if (value.type != JSON_TYPE_BOOLEAN) {
            ERROR(JSONValue, TemplateError, 0, "Can only invert boolean values");
        }
        RETURN(JSONValue, json_bool(!value.boolean));
    }
    case UTONegate: {
        if (value.type != JSON_TYPE_INT) {
            ERROR(JSONValue, TemplateError, 0, "Can only negate integer values");
        }
        RETURN(JSONValue, json_int(-value.int_number.i32));
    }
    default:
        UNREACHABLE();
    }
}

ErrorOrJSONValue evaluate_binary_expression(TemplateRenderContext *ctx, TemplateExpression *expr)
{
    JSONValue lhs = TRY(JSONValue, evaluate_expression(ctx, expr->binary.lhs));
    if (expr->binary.op == BTOSubscript) {
        if (lhs.type != JSON_TYPE_OBJECT) {
            ERROR(JSONValue, TemplateError, 0, "Only objects can be subscripted");
        }
        JSONValue rhs = { 0 };
        if (expr->binary.rhs->type != TETIdentifier) {
            rhs = TRY(JSONValue, evaluate_expression(ctx, expr->binary.rhs));
            if (rhs.type != JSON_TYPE_STRING) {
                ERROR(JSONValue, TemplateError, 0, "Subscript must be identifier");
            }
        } else {
            rhs = json_string(sv(&ctx->sb, expr->binary.rhs->text));
        }
        JSONValue property = TRY_OPTIONAL(JSONValue, json_get_sv(&lhs, rhs.string),
            JSONValue, TemplateError, "Property '%.*s' not there", SV_ARG(rhs.string));
        RETURN(JSONValue, property);
    }

    JSONValue rhs = TRY(JSONValue, evaluate_expression(ctx, expr->binary.rhs));
    switch (expr->binary.op) {
    case BTOAdd: {
        if (lhs.type != JSON_TYPE_INT || rhs.type != JSON_TYPE_INT) {
            ERROR(JSONValue, TemplateError, 0, "Can only add integer values");
        }
        int ret = json_int_value(lhs) + json_int_value(rhs);
        RETURN(JSONValue, json_int(ret));
    }
    case BTOSubtract: {
        if (lhs.type != JSON_TYPE_INT || rhs.type != JSON_TYPE_INT) {
            ERROR(JSONValue, TemplateError, 0, "Can only subtract integer values");
        }
        int ret = json_int_value(lhs) - json_int_value(rhs);
        RETURN(JSONValue, json_int(ret));
    }
    case BTOMultiply: {
        if (lhs.type != JSON_TYPE_INT || rhs.type != JSON_TYPE_INT) {
            ERROR(JSONValue, TemplateError, 0, "Can only multiply integer values");
        }
        int ret = json_int_value(lhs) * json_int_value(rhs);
        RETURN(JSONValue, json_int(ret));
    }
    case BTODivide: {
        if (lhs.type != JSON_TYPE_INT || rhs.type != JSON_TYPE_INT) {
            ERROR(JSONValue, TemplateError, 0, "Can only divide integer values");
        }
        int ret = json_int_value(lhs) / json_int_value(rhs);
        RETURN(JSONValue, json_int(ret));
    }
    case BTOModulo: {
        if (lhs.type != JSON_TYPE_INT || rhs.type != JSON_TYPE_INT) {
            ERROR(JSONValue, TemplateError, 0, "Can only take the modulus of integer values");
        }
        int ret = json_int_value(lhs) % json_int_value(rhs);
        RETURN(JSONValue, json_int(ret));
    }
    case BTOEquals: {
        bool ret;
        if (lhs.type == JSON_TYPE_INT || rhs.type == JSON_TYPE_INT) {
            ret = json_int_value(lhs) == json_int_value(rhs);
        } else if (lhs.type == JSON_TYPE_STRING || rhs.type == JSON_TYPE_STRING) {
            ret = sv_eq(lhs.string, rhs.string);
        } else if (lhs.type == JSON_TYPE_BOOLEAN || rhs.type == JSON_TYPE_BOOLEAN) {
            ret = lhs.boolean == rhs.boolean;
        } else {
            ERROR(JSONValue, TemplateError, 0, "Can't compare your values yet");
        }
        RETURN(JSONValue, json_bool(ret));
    }
    case BTONotEquals: {
        bool ret;
        if (lhs.type == JSON_TYPE_INT || rhs.type == JSON_TYPE_INT) {
            ret = json_int_value(lhs) != json_int_value(rhs);
        } else if (lhs.type == JSON_TYPE_STRING || rhs.type == JSON_TYPE_STRING) {
            ret = !sv_eq(lhs.string, rhs.string);
        } else if (lhs.type == JSON_TYPE_BOOLEAN || rhs.type == JSON_TYPE_BOOLEAN) {
            ret = lhs.boolean != rhs.boolean;
        } else {
            ERROR(JSONValue, TemplateError, 0, "Can't compare your values yet");
        }
        RETURN(JSONValue, json_bool(ret));
    }
    case BTOGreater: {
        bool ret;
        if (lhs.type == JSON_TYPE_INT || rhs.type == JSON_TYPE_INT) {
            ret = json_int_value(lhs) > json_int_value(rhs);
        } else if (lhs.type == JSON_TYPE_STRING || rhs.type == JSON_TYPE_STRING) {
            ret = sv_cmp(lhs.string, rhs.string) > 0;
        } else {
            ERROR(JSONValue, TemplateError, 0, "Can't compare your values yet");
        }
        RETURN(JSONValue, json_bool(ret));
    }
    case BTOGreaterEquals: {
        bool ret;
        if (lhs.type == JSON_TYPE_INT || rhs.type == JSON_TYPE_INT) {
            ret = json_int_value(lhs) >= json_int_value(rhs);
        } else if (lhs.type == JSON_TYPE_STRING || rhs.type == JSON_TYPE_STRING) {
            ret = sv_cmp(lhs.string, rhs.string) >= 0;
        } else {
            ERROR(JSONValue, TemplateError, 0, "Can't compare your values yet");
        }
        RETURN(JSONValue, json_bool(ret));
    }
    case BTOLess: {
        bool ret;
        if (lhs.type == JSON_TYPE_INT || rhs.type == JSON_TYPE_INT) {
            ret = json_int_value(lhs) < json_int_value(rhs);
        } else if (lhs.type == JSON_TYPE_STRING || rhs.type == JSON_TYPE_STRING) {
            ret = sv_cmp(lhs.string, rhs.string) < 0;
        } else {
            ERROR(JSONValue, TemplateError, 0, "Can't compare your values yet");
        }
        RETURN(JSONValue, json_bool(ret));
    }
    case BTOLessEquals: {
        bool ret;
        if (lhs.type == JSON_TYPE_INT || rhs.type == JSON_TYPE_INT) {
            ret = json_int_value(lhs) <= json_int_value(rhs);
        } else if (lhs.type == JSON_TYPE_STRING || rhs.type == JSON_TYPE_STRING) {
            ret = sv_cmp(lhs.string, rhs.string) <= 0;
        } else {
            ERROR(JSONValue, TemplateError, 0, "Can't compare your values yet");
        }
        RETURN(JSONValue, json_bool(ret));
    }
    default:
        UNREACHABLE();
    }
}

ErrorOrJSONValue evaluate_function_call(TemplateRenderContext *ctx, TemplateExpression *expr)
{
    JSONValue  args = json_array();
    StringView name;
    if (expr->function_call.function->type != TETIdentifier) {
        JSONValue name_value = TRY(JSONValue, evaluate_expression(ctx, expr->function_call.function));
        if (name_value.type != JSON_TYPE_STRING) {
            ERROR(JSONValue, TemplateError, 0, "Function name must evaluate to string");
        }
        name = name_value.string;
    } else {
        name = sv(&ctx->sb, expr->function_call.function->text);
    }

    for (size_t ix = 0; ix < expr->function_call.arguments.size; ++ix) {
        JSONValue value = TRY(JSONValue, evaluate_expression(ctx, expr->function_call.arguments.elements[ix]));
        json_append(&args, value);
    }
    for (size_t ix = 0; ix < sizeof(s_functions) / sizeof(TemplateFunctionDescription); ++ix) {
        if (sv_eq_cstr(name, s_functions[ix].name)) {
            return s_functions[ix].function(args);
        }
    }
    ERROR(JSONValue, TemplateError, 0, "Unknown function name '%.*s'", name);
}

ErrorOrJSONValue evaluate_identifier(TemplateRenderContext *ctx, TemplateExpression *expr)
{
    StringView           var_name = sv(&ctx->sb, expr->text);
    TemplateRenderScope *scope = ctx->scope;
    OptionalJSONValue    var = { 0 };
    while (scope) {
        var = json_get_sv(&scope->scope, var_name);
        if (var.has_value) {
            break;
        }
        scope = scope->up;
    }
    if (!var.has_value) {
        ERROR(JSONValue, TemplateError, 0, "Variable '%.*s' not there", SV_ARG(var_name));
    }
    RETURN(JSONValue, var.value);
}

ErrorOrJSONValue evaluate_expression(TemplateRenderContext *ctx, TemplateExpression *expr)
{
    switch (expr->type) {
    case TETUnaryExpression:
        return evaluate_unary_expression(ctx, expr);
    case TETBinaryExpression:
        return evaluate_binary_expression(ctx, expr);
    case TETFunctionCall:
        return evaluate_function_call(ctx, expr);
    case TETBoolean:
        RETURN(JSONValue, json_bool(expr->boolean));
    case TETNull:
        RETURN(JSONValue, json_null());
    case TETNumber:
        RETURN(JSONValue, json_int(expr->number));
    case TETString:
        RETURN(JSONValue, json_string(sv(&ctx->sb, expr->text)));
    case TETIdentifier:
        return evaluate_identifier(ctx, expr);
    default:
        UNREACHABLE();
    }
}

ErrorOrInt render_macro(TemplateRenderContext *ctx, TemplateNode *node)
{
    StringView    name = sv(&ctx->sb, node->macro_call.macro);
    TemplateNode *macro = template_find_macro(ctx->template, node->macro_call.macro);
    assert(macro != NULL);

    TemplateRenderScope call_scope = { json_object(), ctx->scope };
    JSONValue           varargs = { 0 };
    StringView          param_name = { 0 };
    for (size_t ix = 0; ix < macro->macro_def.parameters.size; ++ix) {
        Parameter *param = da_element_Parameter(&macro->macro_def.parameters, ix);
        param_name = sv(&ctx->sb, param->key);
        JSONValue value = TRY_TO(JSONValue, Int, evaluate_expression(ctx, node->macro_call.arguments.elements[ix]));
        if (ix == macro->macro_def.parameters.size - 1 && param->value == JSON_TYPE_ARRAY && value.type != JSON_TYPE_ARRAY) {
            varargs = json_array();
            json_append(&varargs, value);
        } else {
            if (value.type != param->value) {
                ERROR(Int, TemplateError, 0, "Type mismatch for parameter '%.*s' of macro '%.*s'", SV_ARG(param_name), SV_ARG(name));
            }
            json_set_sv(&call_scope.scope, param_name, value);
        }
    }
    if (varargs.type == JSON_TYPE_ARRAY) {
        for (size_t ix = macro->macro_def.parameters.size; ix < node->macro_call.arguments.size; ++ix) {
            JSONValue value = TRY_TO(JSONValue, Int, evaluate_expression(ctx, node->macro_call.arguments.elements[ix]));
            json_append(&varargs, value);
        }
        json_set_sv(&call_scope.scope, param_name, varargs);
    }

    ctx->scope = &call_scope;
    TRY(Int, render_node(ctx, macro->macro_def.contents));
    ctx->scope = call_scope.up;
    RETURN(Int, 0);
}

ErrorOrInt render_expr_node(TemplateRenderContext *ctx, TemplateNode *node)
{
    trace(CAT_TEMPLATE, "Rendering expression node");
    JSONValue  value = TRY_TO(JSONValue, Int, evaluate_expression(ctx, node->expr));
    StringView sv = json_to_string(value);
    sb_append_sv(&ctx->output, sv);
    sv_free(sv);
    RETURN(Int, 0);
}

ErrorOrInt render_for_loop(TemplateRenderContext *ctx, TemplateNode *node)
{
    trace(CAT_TEMPLATE, "Rendering for loop");
    JSONValue value = TRY_TO(JSONValue, Int, evaluate_expression(ctx, node->for_statement.range));
    if (node->for_statement.variable2.length == 0 && value.type != JSON_TYPE_ARRAY) {
        ERROR(Int, TemplateError, 0, "Can only iterate over arrays");
    }
    if (node->for_statement.variable2.length > 0 && value.type != JSON_TYPE_OBJECT) {
        ERROR(Int, TemplateError, 0, "Can only iterate over objects");
    }
    StringView var = sv(&ctx->sb, node->for_statement.variable);
    StringView var2 = sv(&ctx->sb, node->for_statement.variable2);
    for (int ix = 0; ix < json_len(&value); ++ix) {
        TemplateRenderScope loop_scope = { json_object(), ctx->scope };
        ctx->scope = &loop_scope;
        if (var2.length == 0) {
            JSONValue loop_val = MUST_OPTIONAL(JSONValue, json_at(&value, ix));
            json_set_sv(&loop_scope.scope, var, loop_val);
        } else {
            JSONValue loop_vals = MUST_OPTIONAL(JSONValue, json_entry_at(&value, ix));
            json_set_sv(&loop_scope.scope, var, MUST_OPTIONAL(JSONValue, json_at(&loop_vals, 0)));
            json_set_sv(&loop_scope.scope, var2, MUST_OPTIONAL(JSONValue, json_at(&loop_vals, 1)));
        }
        TemplateRenderScope inner_scope = { json_object(), &loop_scope };
        ctx->scope = &inner_scope;
        TRY(Int, render_node(ctx, node->for_statement.contents));
        ctx->scope = ctx->scope->up->up;
    }
    RETURN(Int, 0);
}

ErrorOrInt render_if_statement(TemplateRenderContext *ctx, TemplateNode *node)
{
    trace(CAT_TEMPLATE, "Rendering if statement");
    JSONValue condition = TRY_TO(JSONValue, Int, evaluate_expression(ctx, node->if_statement.condition));
    if (condition.type != JSON_TYPE_BOOLEAN) {
        ERROR(Int, TemplateError, 0, "if condition must be boolean");
    }

    TemplateRenderScope block_scope = { json_object(), ctx->scope };
    ctx->scope = &block_scope;
    TemplateNode *branch = condition.boolean ? node->if_statement.true_branch : node->if_statement.false_branch;
    TRY(Int, render_node(ctx, branch));
    ctx->scope = block_scope.up;
    RETURN(Int, 0);
}

ErrorOrInt render_node(TemplateRenderContext *ctx, TemplateNode *node)
{
    while (node) {
        switch (node->kind) {
        case TNKText: {
            trace(CAT_TEMPLATE, "Rendering text node");
            sb_append_sv(&ctx->output, sv(&ctx->sb, node->text));
        } break;
        case TNKExpr:
            TRY(Int, render_expr_node(ctx, node));
            break;
        case TNKForLoop:
            TRY(Int, render_for_loop(ctx, node));
            break;
        case TNKIfStatement:
            TRY(Int, render_if_statement(ctx, node));
            break;
        case TNKMacroCall:
            TRY(Int, render_macro(ctx, node));
            break;
        case TNKMacroDef:
            break;
        case TNKSetVariable: {
            JSONValue  value = TRY_TO(JSONValue, Int, evaluate_expression(ctx, node->set_statement.value));
            StringView var = sv(&ctx->sb, node->for_statement.variable);
            json_set_sv(&ctx->scope->scope, var, value);
            trace(CAT_TEMPLATE, "Setting variable '%.*s' to '%.*s'", SV_ARG(var), SV_ARG(json_to_string(value)));
        } break;
        default:
            UNREACHABLE();
        }
        node = node->next;
    }
    RETURN(Int, 0);
}

ErrorOrStringView template_render(Template template, JSONValue context)
{
    TemplateRenderContext ctx = { 0 };
    ctx.template = template;
    TemplateRenderScope scope = { json_object(), NULL };
    ctx.scope = &scope;
    json_set(&scope.scope, "$", context);

    TRY_TO(Int, StringView, render_node(&ctx, template.node));
    RETURN(StringView, ctx.output.view);
}

static ErrorOrJSONValue template_function_toupper(JSONValue args)
{
    if (args.type != JSON_TYPE_ARRAY) {
        ERROR(JSONValue, TemplateError, 0, "Arguments must be passed to template functions as arrays");
    }
    if (json_len(&args) != 1) {
        ERROR(JSONValue, TemplateError, 0, "Template function toupper expects only a single argument");
    }
    JSONValue s = MUST_OPTIONAL(JSONValue, json_at(&args, 0));
    if (s.type != JSON_TYPE_STRING) {
        ERROR(JSONValue, TemplateError, 0, "Argument of template function toupper must be string");
    }
    static StringBuilder sb;
    sb.length = 0;
    for (size_t ix = 0; ix < s.string.length; ++ix) {
        sb_append_char(&sb, toupper(s.string.ptr[ix]));
    }
    RETURN(JSONValue, json_string(sb.view));
}
