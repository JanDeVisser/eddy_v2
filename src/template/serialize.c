/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <template/template.h>

char const *TemplateNodeKind_name(TemplateNodeKind kind)
{
    switch (kind) {
#undef KIND
#define KIND(K)  \
    case TNK##K: \
        return #K;
        TEMPLATENODEKINDS(KIND)
#undef KIND
    default:
        UNREACHABLE();
    }
}

char const *TemplateExpressionType_name(TemplateExpressionType type)
{
    switch (type) {
#undef TYPE
#define TYPE(T)  \
    case TET##T: \
        return #T;
        TEMPLATEEXPRESSIONTYPES(TYPE)
#undef KIND
    default:
        UNREACHABLE();
    }
}

char const *TplOperator_name(TplOperator op)
{
    switch (op) {
    case InvalidOperator:
        return "InvalidOperator";
    case BTOCall:
        return "BTOCall";
    case BTOSubscript:
        return "BTOSubscript";
    case BTODereference:
        return "BTODereference";
    case BTOMultiply:
        return "BTOMultiply";
    case BTODivide:
        return "BTODivide";
    case BTOModulo:
        return "BTOModulo";
    case BTOAdd:
        return "BTOAdd";
    case BTOSubtract:
        return "BTOSubtract";
    case BTOGreater:
        return "BTOGreater";
    case BTOGreaterEquals:
        return "BTOGreaterEquals";
    case BTOLess:
        return "BTOLess";
    case BTOLessEquals:
        return "BTOLessEquals";
    case BTOEquals:
        return "BTOEquals";
    case BTONotEquals:
        return "BTONotEquals";
    case UTOIdentity:
        return "UTOIdentity";
    case UTONegate:
        return "UTONegate";
    case UTOInvert:
        return "UTOInvert";
    case UTODereference:
        return "UTODereference";
    case UTOParenthesize:
        return "UTOParenthesize";
    default:
        UNREACHABLE();
    }
}

char const *TplKeyword_name(TplKeyword keyword)
{
    switch (keyword) {
#undef S
#define S(T, STR, ALT)  \
    case TPLKW##T: \
        return #T;
        TPLKEYWORDS(S)
#undef S
    default:
        UNREACHABLE();
    }
}

JSONValue template_expression_serialize(Template tpl, TemplateExpression *expr)
{
    JSONValue   ret = json_object();
    char const *t = TemplateExpressionType_name(expr->type);
    json_set_cstr(&ret, "type", t);
    switch (expr->type) {
    case TETUnaryExpression: {
        JSONValue unary = json_object();
        json_set_cstr(&unary, "operator", TplOperator_name(expr->unary.op));
        json_set(&unary, "operand", template_expression_serialize(tpl, expr->unary.operand));
        json_set(&ret, t, unary);
    } break;
    case TETBinaryExpression: {
        JSONValue binary = json_object();
        json_set(&binary, "lhs", template_expression_serialize(tpl, expr->binary.lhs));
        json_set_cstr(&binary, "operator", TplOperator_name(expr->binary.op));
        json_set(&binary, "rhs", template_expression_serialize(tpl, expr->binary.rhs));
        json_set(&ret, t, binary);
    } break;
    case TETBoolean:
        json_set(&ret, t, json_bool(expr->boolean));
        break;
    case TETNull:
        json_set(&ret, t, json_null());
        break;
    case TETNumber:
        json_set(&ret, t, json_int(expr->number));
        break;
    case TETString:
        json_set(&ret, t, json_string(sv(&tpl.sb, expr->text)));
        break;
    case TETIdentifier:
        json_set(&ret, t, json_string(expr->raw_text));
        break;
    case TETFunctionCall: {
        JSONValue call = json_object();
        json_set(&call, "lhs", template_expression_serialize(tpl, expr->function_call.function));
        JSONValue args = json_array();
        for (size_t ix = 0; ix < expr->function_call.arguments.size; ++ix) {
            json_append(&args, template_expression_serialize(tpl, expr->function_call.arguments.elements[ix]));
        }
        json_set(&call, "arguments", args);
        json_set(&ret, t, call);
    } break;
    default:
        UNREACHABLE();
    }
    return ret;
}
JSONValue template_node_serialize(Template tpl, TemplateNode *node)
{
    JSONValue ret = json_array();
    while (node) {
        JSONValue   n = json_object();
        char const *k = TemplateNodeKind_name(node->kind);
        json_set_cstr(&n, "kind", k);
        switch (node->kind) {
        case TNKText: {
            json_set_string(&n, k, sv(&tpl.sb, node->text));
        } break;
        case TNKExpr: {
            assert(node->expr != NULL);
            json_set(&n, k, template_expression_serialize(tpl, node->expr));
        } break;
        case TNKForLoop: {
            JSONValue for_loop = json_object();
            json_set_string(&for_loop, "key_variable", node->for_statement.variable);
            json_set_string(&for_loop, "value_variable", node->for_statement.variable2);
            json_set(&for_loop, "range", template_expression_serialize(tpl, node->for_statement.range));
            json_set(&for_loop, "contents", template_node_serialize(tpl, node->contents));
            json_set(&n, k, for_loop);
        } break;
        case TNKIfStatement: {
            JSONValue if_stmt = json_object();
            json_set(&if_stmt, "condition", template_expression_serialize(tpl, node->if_statement.condition));
            json_set(&if_stmt, "on_true", template_node_serialize(tpl, node->if_statement.true_branch));
            json_set(&if_stmt, "on_false", template_node_serialize(tpl, node->if_statement.false_branch));
            json_set(&n, k, if_stmt);
        } break;
        case TNKMacroCall: {
            JSONValue macro_call = json_object();
            json_set(&macro_call, "macro", json_string(node->macro_call.macro));
            JSONValue args = json_array();
            for (size_t ix = 0; ix < node->macro_call.arguments.size; ++ix) {
                json_append(&args, template_expression_serialize(tpl, node->macro_call.arguments.elements[ix]));
            }
            json_set(&macro_call, "arguments", args);
            json_set(&macro_call, "contents", template_node_serialize(tpl, node->contents));
            json_set(&n, k, macro_call);
        } break;
        case TNKMacroDef: {
            JSONValue macro_def = json_object();
            json_set(&macro_def, "name", json_string(node->macro_def.name));
            JSONValue params = json_array();
            for (size_t ix = 0; ix < node->macro_def.parameters.size; ++ix) {
                Parameter p = node->macro_def.parameters.elements[ix];
                JSONValue param = json_object();
                json_set_string(&param, "name", p.key);
                json_set_cstr(&param, "type", JSONType_name(p.value));
                json_append(&params, param);
            }
            json_set(&macro_def, "parameters", params);
            json_set(&macro_def, "contents", template_node_serialize(tpl, node->contents));
            json_set(&n, k, macro_def);
        } break;
        case TNKSetVariable: {
            JSONValue set_var = json_object();
            json_set(&set_var, "variable", json_string(node->set_statement.variable));
            json_set(&set_var, "value", template_expression_serialize(tpl, node->set_statement.value));
            json_set(&n, k, set_var);
        } break;
        default:
            UNREACHABLE();
        }
        json_append(&ret, n);
        node = node->next;
    }
    return ret;
}
