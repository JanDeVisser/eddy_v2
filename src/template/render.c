/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "error_or.h"
#include "json.h"
#include <ctype.h>

#include <template/template.h>

typedef struct template_render_scope {
    JSONValue                     scope;
    struct template_render_scope *up;
    StringView                    s;
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

typedef ErrorOrJSONValue (*TemplateFunction)(TemplateRenderContext *, JSONValue);

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
static ErrorOrJSONValue template_function_render(TemplateRenderContext *ctx, JSONValue args);
static ErrorOrJSONValue template_function_toupper(TemplateRenderContext *ctx, JSONValue args);
static ErrorOrJSONValue template_function_tolower(TemplateRenderContext *ctx, JSONValue args);

TemplateFunctionDescription s_functions[] = {
    { "render", template_function_render },
    { "toupper", template_function_toupper },
    { "tolower", template_function_tolower },
};

ErrorOrJSONValue evaluate_unary_expression(TemplateRenderContext *ctx, TemplateExpression *expr)
{
    JSONValue value = TRY(JSONValue, evaluate_expression(ctx, expr->unary.operand));
    switch (expr->unary.op) {
    case UTODereference:
        if (value.type != JSON_TYPE_STRING) {
            json_free(value);
            ERROR(JSONValue, TemplateError, 0, "{...} must evaluate to a string");
        }
        RETURN(JSONValue, value);
    case UTOIdentity:
    case UTOParenthesize:
        RETURN(JSONValue, value);
    case UTOInvert: {
        if (value.type != JSON_TYPE_BOOLEAN) {
            json_free(value);
            ERROR(JSONValue, TemplateError, 0, "Can only invert boolean values");
        }
        RETURN(JSONValue, json_bool(!value.boolean));
    }
    case UTONegate: {
        if (value.type != JSON_TYPE_INT) {
            json_free(value);
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
    ErrorOrJSONValue ret = { 0 };
    JSONValue        lhs = TRY(JSONValue, evaluate_expression(ctx, expr->binary.lhs));
    trace(TEMPLATE, "Rendering binary expression %s", TplOperator_name(expr->binary.op));
    if (expr->binary.op == BTOSubscript) {
        if (lhs.type != JSON_TYPE_OBJECT) {
            json_free(lhs);
            ERROR(JSONValue, TemplateError, 0, "Only objects can be subscripted");
        }
        trace(TEMPLATE, "lhs: %.*s", SV_ARG(json_to_string(lhs)));
        JSONValue rhs = { 0 };
        if (expr->binary.rhs->type != TETIdentifier) {
            rhs = TRY(JSONValue, evaluate_expression(ctx, expr->binary.rhs));
            if (rhs.type != JSON_TYPE_STRING) {
                json_free(rhs);
                json_free(lhs);
                ERROR(JSONValue, TemplateError, 0, "Subscript must be identifier");
            }
        } else {
            rhs = json_string(expr->binary.rhs->raw_text);
        }
        OptionalJSONValue property_maybe = json_get_sv(&lhs, rhs.string);
        if (!property_maybe.has_value) {
            json_free(rhs);
            json_free(lhs);
            ERROR(JSONValue, TemplateError, 0, "Property '%.*s' not there", SV_ARG(rhs.string));
        }
        JSONValue property = json_copy(property_maybe.value);
        trace(TEMPLATE, ".[%.*s] = '%.*s'", SV_ARG(rhs.string), SV_ARG(json_to_string(property)));
        json_free(rhs);
        json_free(lhs);
        RETURN(JSONValue, property);
    }

    JSONValue rhs = TRY(JSONValue, evaluate_expression(ctx, expr->binary.rhs));
    switch (expr->binary.op) {
    case BTOAdd: {
        if (lhs.type != JSON_TYPE_INT || rhs.type != JSON_TYPE_INT) {
            ret = ErrorOrJSONValue_error(__FILE_NAME__, __LINE__, TemplateError, 0, "Can only add integer values");
            goto defer_rhs;
        }
        int retval = json_int_value(lhs) + json_int_value(rhs);
        ret = (ErrorOrJSONValue) { .error = { 0 }, .value = json_int(retval) };
        goto defer_rhs;
    }
    case BTOSubtract: {
        if (lhs.type != JSON_TYPE_INT || rhs.type != JSON_TYPE_INT) {
            ret = ErrorOrJSONValue_error(__FILE_NAME__, __LINE__, TemplateError, 0, "Can only subtract integer values");
            goto defer_rhs;
        }
        int retval = json_int_value(lhs) - json_int_value(rhs);
        ret = (ErrorOrJSONValue) { .error = { 0 }, .value = json_int(retval) };
        goto defer_rhs;
    }
    case BTOMultiply: {
        if (lhs.type != JSON_TYPE_INT || rhs.type != JSON_TYPE_INT) {
            ret = ErrorOrJSONValue_error(__FILE_NAME__, __LINE__, TemplateError, 0, "Can only multiply integer values");
            goto defer_rhs;
        }
        int retval = json_int_value(lhs) * json_int_value(rhs);
        ret = (ErrorOrJSONValue) { .error = { 0 }, .value = json_int(retval) };
        goto defer_rhs;
    }
    case BTODivide: {
        if (lhs.type != JSON_TYPE_INT || rhs.type != JSON_TYPE_INT) {
            ret = ErrorOrJSONValue_error(__FILE_NAME__, __LINE__, TemplateError, 0, "Can only divide integer values");
            goto defer_rhs;
        }
        int retval = json_int_value(lhs) / json_int_value(rhs);
        ret = (ErrorOrJSONValue) { .error = { 0 }, .value = json_int(retval) };
        goto defer_rhs;
    }
    case BTOModulo: {
        if (lhs.type != JSON_TYPE_INT || rhs.type != JSON_TYPE_INT) {
            ret = ErrorOrJSONValue_error(__FILE_NAME__, __LINE__, TemplateError, 0, "Can only take the modulus of integer values");
            goto defer_rhs;
        }
        int retval = json_int_value(lhs) % json_int_value(rhs);
        ret = (ErrorOrJSONValue) { .error = { 0 }, .value = json_int(retval) };
        goto defer_rhs;
    }
    case BTOEquals: {
        bool retval = json_compare(lhs, rhs) == 0;
        ret = (ErrorOrJSONValue) { .error = { 0 }, .value = json_bool(retval) };
        goto defer_rhs;
    }
    case BTONotEquals: {
        bool retval = json_compare(lhs, rhs) != 0;
        ret = (ErrorOrJSONValue) { .error = { 0 }, .value = json_bool(retval) };
        goto defer_rhs;
    }
    case BTOGreater: {
        bool retval = json_compare(lhs, rhs) > 0;
        ret = (ErrorOrJSONValue) { .error = { 0 }, .value = json_bool(retval) };
        goto defer_rhs;
    }
    case BTOGreaterEquals: {
        bool retval = json_compare(lhs, rhs) >= 0;
        ret = (ErrorOrJSONValue) { .error = { 0 }, .value = json_bool(retval) };
        goto defer_rhs;
    }
    case BTOLess: {
        bool retval = json_compare(lhs, rhs) < 0;
        ret = (ErrorOrJSONValue) { .error = { 0 }, .value = json_bool(retval) };
        goto defer_rhs;
    }
    case BTOLessEquals: {
        bool retval = json_compare(lhs, rhs) <= 0;
        ret = (ErrorOrJSONValue) { .error = { 0 }, .value = json_bool(retval) };
        goto defer_rhs;
    }
    default:
        UNREACHABLE();
    }
defer_rhs:
    json_free(rhs);
defer_lhs:
    json_free(lhs);
    return ret;
}

ErrorOrJSONValue evaluate_function_call(TemplateRenderContext *ctx, TemplateExpression *expr)
{
    JSONValue  args = json_array();
    StringView name;
    JSONValue  name_value = { 0 };
    if (expr->function_call.function->type == TETIdentifier) {
        name = expr->function_call.function->raw_text;
    } else {
        name_value = TRY(JSONValue, evaluate_expression(ctx, expr->function_call.function));
        if (name_value.type != JSON_TYPE_STRING) {
            ERROR(JSONValue, TemplateError, 0, "Function name must evaluate to string");
        }
        name = name_value.string;
    }

    for (size_t ix = 0; ix < expr->function_call.arguments.size; ++ix) {
        JSONValue value = TRY(JSONValue, evaluate_expression(ctx, expr->function_call.arguments.elements[ix]));
        json_append(&args, value);
    }
    for (size_t ix = 0; ix < sizeof(s_functions) / sizeof(TemplateFunctionDescription); ++ix) {
        if (sv_eq_cstr(name, s_functions[ix].name)) {
            ErrorOrJSONValue ret = s_functions[ix].function(ctx, args);
            json_free(args);
            json_free(name_value);
            return ret;
        }
    }
    json_free(args);
    ErrorOrJSONValue ret = ErrorOrJSONValue_error(__FILE_NAME__, __LINE__, TemplateError, 0, "Unknown function name '%.*s'", name);
    json_free(name_value);
    return ret;
}

JSONValue *get_variable_scope(TemplateRenderContext *ctx, StringView name)
{
    TemplateRenderScope *scope = ctx->scope;
    while (scope) {
        JSONValue *var = json_get_ref(&scope->scope, name);
        if (var != NULL) {
            return &scope->scope;
        }
        scope = scope->up;
    }
    return NULL;
}

ErrorOrJSONValue evaluate_identifier(TemplateRenderContext *ctx, TemplateExpression *expr)
{
    JSONValue *scope = get_variable_scope(ctx, expr->raw_text);
    if (scope == NULL) {
        ERROR(JSONValue, TemplateError, 0, "Variable '%.*s' not there", SV_ARG(expr->raw_text));
    }
    RETURN(JSONValue, json_copy(MUST_OPTIONAL(JSONValue, json_get_sv(scope, expr->raw_text))));
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

ErrorOrInt call_macro(TemplateRenderContext *ctx, StringView name, JSONValue args, TemplateNode *contents)
{
    TemplateNode *macro = template_find_macro(ctx->template, name);
    if (macro == NULL) {
        ERROR(Int, TemplateError, 0, "Unknown macro '%.*s' called", SV_ARG(name));
    }

    for (size_t ix = 0; ix < macro->macro_def.parameters.size; ++ix) {
        Parameter        *param = da_element_Parameter(&macro->macro_def.parameters, ix);
        StringView        param_name = param->key;
        OptionalJSONValue value_maybe = json_get_sv(&args, param_name);
        if (!value_maybe.has_value) {
            json_free(args);
            ERROR(Int, TemplateError, 0, "Missing argument '%.*s' in call of macro '%.*s'", SV_ARG(param_name), SV_ARG(name));
        }
        if (value_maybe.value.type != param->value) {
            json_free(args);
            ERROR(Int, TemplateError, 0, "Type mismatch for parameter '%.*s' of macro '%.*s'", SV_ARG(param_name), SV_ARG(name));
        }
    }

    TemplateRenderScope arg_scope = { args, ctx->scope };
    TemplateRenderScope call_scope = { json_object(), &arg_scope };
    json_set(&call_scope.scope, "$contents", json_integer((Integer) { U64, .u64 = (intptr_t) contents }));
    ctx->scope = &call_scope;
    TRY(Int, render_node(ctx, macro->contents));
    ctx->scope = arg_scope.up;
    json_free(call_scope.scope);
    RETURN(Int, 1);
}

ErrorOrInt render_macro(TemplateRenderContext *ctx, TemplateNode *node)
{
    if (node->macro_call.condition != NULL) {
        JSONValue condition = TRY_TO(JSONValue, Int, evaluate_expression(ctx, node->macro_call.condition));
        if (condition.type != JSON_TYPE_BOOLEAN) {
            ERROR(Int, TemplateError, 0, "call condition must be boolean");
        }
        if (!condition.boolean) {
            RETURN(Int, 0);
        }
    }

    TemplateNode *macro = template_find_macro(ctx->template, node->macro_call.macro);
    assert(macro != NULL);

    JSONValue args = json_object();
    JSONValue varargs = { 0 };
    for (size_t ix = 0; ix < macro->macro_def.parameters.size; ++ix) {
        Parameter *param = da_element_Parameter(&macro->macro_def.parameters, ix);
        JSONValue  value = TRY_TO(JSONValue, Int, evaluate_expression(ctx, node->macro_call.arguments.elements[ix]));
        if (ix == macro->macro_def.parameters.size - 1 && param->value == JSON_TYPE_ARRAY && value.type != JSON_TYPE_ARRAY) {
            varargs = json_array();
            json_append(&varargs, value);
        } else {
            json_set_sv(&args, param->key, value);
        }
    }
    if (varargs.type == JSON_TYPE_ARRAY) {
        for (size_t ix = macro->macro_def.parameters.size; ix < node->macro_call.arguments.size; ++ix) {
            JSONValue value = TRY_TO(JSONValue, Int, evaluate_expression(ctx, node->macro_call.arguments.elements[ix]));
            json_append(&varargs, value);
        }
        json_set_sv(&args, macro->macro_def.parameters.elements[macro->macro_def.parameters.size - 1].key, varargs);
    }
    return call_macro(ctx, node->macro_call.macro, args, node->contents);
}

ErrorOrInt render_expr_node(TemplateRenderContext *ctx, TemplateNode *node)
{
    JSONValue  value = TRY_TO(JSONValue, Int, evaluate_expression(ctx, node->expr));
    StringView sv = json_to_string(value);
    trace(TEMPLATE, "Rendering expression node '%.*s'", SV_ARG(sv));
    sb_append_sv(&ctx->output, sv);
    sv_free(sv);
    json_free(value);
    RETURN(Int, 0);
}

ErrorOrInt render_for_loop(TemplateRenderContext *ctx, TemplateNode *node)
{
    trace(TEMPLATE, "Rendering for loop");
    JSONValue value = TRY_TO(JSONValue, Int, evaluate_expression(ctx, node->for_statement.range));
    if (node->for_statement.variable2.length == 0 && value.type != JSON_TYPE_ARRAY) {
        ERROR(Int, TemplateError, 0, "Can only iterate over arrays");
    }
    if (node->for_statement.variable2.length > 0 && value.type != JSON_TYPE_OBJECT) {
        ERROR(Int, TemplateError, 0, "Can only iterate over objects");
    }
    StringView          var = node->for_statement.variable;
    StringView          var2 = node->for_statement.variable2;
    TemplateRenderScope loop_scope = { json_object(), ctx->scope };
    ctx->scope = &loop_scope;
    for (int ix = 0; ix < json_len(&value); ++ix) {
        json_set(&loop_scope.scope, "$index", json_int(ix));
        if (var2.length == 0) {
            JSONValue loop_val = MUST_OPTIONAL(JSONValue, json_at(&value, ix));
            json_set_sv(&loop_scope.scope, var, json_copy(loop_val));
        } else {
            JSONValue loop_vals = MUST_OPTIONAL(JSONValue, json_entry_at(&value, ix));
            json_set_sv(&loop_scope.scope, var, json_copy(MUST_OPTIONAL(JSONValue, json_at(&loop_vals, 0))));
            json_set_sv(&loop_scope.scope, var2, json_copy(MUST_OPTIONAL(JSONValue, json_at(&loop_vals, 1))));
        }

        if (node->for_statement.condition != NULL) {
            JSONValue include_value = TRY_TO(JSONValue, Int, evaluate_expression(ctx, node->for_statement.condition));
            if (include_value.type != JSON_TYPE_BOOLEAN) {
                json_free(value);
                ERROR(Int, TemplateError, 0, "for loop condition must be boolean. '%.*s' is a %s", SV_ARG(json_to_string(include_value)), JSONType_name(include_value.type));
            }
            if (!include_value.boolean) {
                continue;
            }
        }

        if (node->for_statement.macro.length == 0) {
            TemplateRenderScope inner_scope = { json_object(), &loop_scope };
            ctx->scope = &inner_scope;
            TRY(Int, render_node(ctx, node->contents));
            json_free(inner_scope.scope);
            ctx->scope = &loop_scope;
        } else {
            ctx->scope = ctx->scope->up;
            TRY(Int, call_macro(ctx, node->for_statement.macro, loop_scope.scope, node->contents));
            ctx->scope = &loop_scope;
        }
    }
    ctx->scope = ctx->scope->up;
    json_free(value);
    RETURN(Int, 0);
}

ErrorOrInt render_if_statement(TemplateRenderContext *ctx, TemplateNode *node)
{
    trace(TEMPLATE, "Rendering if statement");
    JSONValue condition = TRY_TO(JSONValue, Int, evaluate_expression(ctx, node->if_statement.condition));
    if (condition.type != JSON_TYPE_BOOLEAN) {
        ERROR(Int, TemplateError, 0, "if condition must be boolean");
    }

    TemplateRenderScope block_scope = { json_object(), ctx->scope };
    ctx->scope = &block_scope;
    TemplateNode *branch = condition.boolean ? node->if_statement.true_branch : node->if_statement.false_branch;
    TRY(Int, render_node(ctx, branch));
    json_free(block_scope.scope);
    ctx->scope = block_scope.up;
    RETURN(Int, 0);
}

ErrorOrInt render_switch_statement(TemplateRenderContext *ctx, TemplateNode *node)
{
    trace(TEMPLATE, "Rendering switch statement");
    for (TemplateNode *case_node = node->switch_statement.cases; case_node; case_node = case_node->next) {
        // 0: skipped, 1: executed:
        int executed = TRY(Int, render_macro(ctx, case_node));
        if (executed) {
            break;
        }
    }
    RETURN(Int, 0);
}

ErrorOrInt render_node(TemplateRenderContext *ctx, TemplateNode *node)
{
    while (node) {
        switch (node->kind) {
        case TNKText: {
            StringView s = sv(&ctx->sb, node->text);
            trace(TEMPLATE, "Rendering text node '%.*s'", SV_ARG(s));
            sb_append_sv(&ctx->output, s);
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
            JSONValue *scope = get_variable_scope(ctx, node->set_statement.variable);
            JSONValue  value = TRY_TO(JSONValue, Int, evaluate_expression(ctx, node->set_statement.value));
            if (scope == NULL) {
                scope = &ctx->scope->scope;
            }
            json_set_sv(scope, node->set_statement.variable, value);
            trace(TEMPLATE, "Setting variable '%.*s' to '%.*s'", SV_ARG(node->for_statement.variable), SV_ARG(json_to_string(value)));
        } break;
        case TNKSwitchStatement:
            TRY(Int, render_switch_statement(ctx, node));
            break;
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

static ErrorOrJSONValue template_function_render(TemplateRenderContext *ctx, JSONValue args)
{
    if (args.type != JSON_TYPE_ARRAY) {
        ERROR(JSONValue, TemplateError, 0, "Arguments must be passed to template functions as arrays");
    }
    if (json_len(&args) != 1) {
        ERROR(JSONValue, TemplateError, 0, "Template function render expects only a single argument");
    }
    JSONValue s = MUST_OPTIONAL(JSONValue, json_at(&args, 0));
    if (s.type != JSON_TYPE_INT || s.int_number.type != U64) {
        ERROR(JSONValue, TemplateError, 0, "Argument of template function render must be unsigned 64-bit integer");
    }
    TemplateNode *node = (TemplateNode *) s.int_number.u64;
    TRY_TO(Int, JSONValue, render_node(ctx, node));
    RETURN(JSONValue, json_null());
}

static ErrorOrJSONValue template_function_tolower(TemplateRenderContext *ctx, JSONValue args)
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
        sb_append_char(&sb, tolower(s.string.ptr[ix]));
    }
    RETURN(JSONValue, json_string(sb.view));
}

static ErrorOrJSONValue template_function_toupper(TemplateRenderContext *ctx, JSONValue args)
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
