/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <model/syntaxnode.h>

#undef SYNTAXNODETYPE
#define SYNTAXNODETYPE(type) \
    __attribute__((unused)) void type##_to_json(SyntaxNode *node, JSONValue *json);
SYNTAXNODETYPES(SYNTAXNODETYPE)
#undef SYNTAXNODETYPE

char const *SyntaxNodeType_name(SyntaxNodeType type)
{
    switch (type) {
#undef SYNTAXNODETYPE_ENUM
#define SYNTAXNODETYPE_ENUM(type) \
    case SNT_##type:              \
        return "SNT_" #type;
        SYNTAXNODETYPES(SYNTAXNODETYPE_ENUM)
#undef SYNTAXNODETYPE_ENUM
    default:
        UNREACHABLE();
    }
}

SyntaxNodeType SyntaxNodeType_from_string(StringView type)
{
#undef SYNTAXNODETYPE_ENUM
#define SYNTAXNODETYPE_ENUM(T)            \
    if (sv_eq_ignore_case_cstr(type, #T)) \
        return SNT_##T;
    SYNTAXNODETYPES(SYNTAXNODETYPE_ENUM)
#undef SYNTAXNODETYPE_ENUM
    fatal("Invalid syntax node type '%.*s'", SV_ARG(type));
}

JSONValue type_descr_to_json(TypeDescr *type)
{
    JSONValue ret = json_object();
    json_set_string(&ret, "name", type->name);
    JSONValue components = json_array();
    for (size_t ix = 0; ix < type->size; ++ix) {
        json_append(&components, type_descr_to_json(type->elements[ix]));
    }
    json_set(&ret, "components", components);
    return ret;
}

JSONValue node_facade(SyntaxNode *node)
{
    if (node != NULL) {
        JSONValue facade = json_object();
        json_set_string(&facade, "name", node->name);
        json_set_cstr(&facade, "nodetype", SyntaxNodeType_name(node->type));
        json_set(&facade, "token", token_to_json(node->token));
        json_set_int(&facade, "index", node->index);
        return facade;
    }
    return json_null();
}

void node_facade_sub(SyntaxNode *node, JSONValue *json, char const *name)
{
    if (node != NULL) {
        json_set(json, name, node_facade(node));
    }
}

void node_chain_to_array(SyntaxNode *chain, JSONValue *json, char const *attr)
{
    JSONValue ret = json_array();
    for (SyntaxNode *n = chain; n; n = n->next) {
        json_append(&ret, node_facade(n));
    }
    json_set(json, attr, ret);
}

void ASSIGNMENT_to_json(SyntaxNode *node, JSONValue *json)
{
    json_set_cstr(json, "operator", Operator_name(node->assignment.operator));
    node_facade_sub(node->assignment.expression, json, "expression");
}

void BINARYEXPRESSION_to_json(SyntaxNode *node, JSONValue *json)
{
    node_facade_sub(node->binary_expr.lhs, json, "lhs");
    json_set_cstr(json, "operator", Operator_name(node->binary_expr.operator));
    node_facade_sub(node->binary_expr.rhs, json, "rhs");
}

void BLOCK_to_json(SyntaxNode *node, JSONValue *json)
{
    node_chain_to_array(node->block.statements, json, "statements");
}

void BOOL_to_json(SyntaxNode *node, JSONValue *json)
{
    json_set(json, "value", json_bool(sv_eq_ignore_case_cstr(node->name, "true")));
}

void BREAK_to_json(SyntaxNode *, JSONValue *)
{
}

void COMPOUND_to_json(SyntaxNode *node, JSONValue *json)
{
    node_chain_to_array(node->compound_expr.expressions, json, "expressions");
}

void CONTINUE_to_json(SyntaxNode *, JSONValue *)
{
}

void DECIMAL_to_json(SyntaxNode *node, JSONValue *json)
{
}

void ENUMERATION_to_json(SyntaxNode *node, JSONValue *json)
{
    node_facade_sub(node->enumeration.underlying_type, json, "underlying_type");
    node_chain_to_array(node->enumeration.values, json, "values");
}

void ENUM_VALUE_to_json(SyntaxNode *node, JSONValue *json)
{
    if (node->enum_value.underlying_value) {
        node_facade_sub(node->enum_value.underlying_value, json, "underlying_value");
    }
}

void FOR_to_json(SyntaxNode *node, JSONValue *json)
{
    json_set_string(json, "variable", node->for_statement.variable);
    node_facade_sub(node->for_statement.range, json, "range");
    node_facade_sub(node->for_statement.statement, json, "statement");
}

void FUNCTION_to_json(SyntaxNode *node, JSONValue *json)
{
    node_chain_to_array(node->function.parameter, json, "parameters");
    node_facade_sub(node->function.return_type, json, "return_type");
    node_facade_sub(node->function.error_type, json, "error_type");
    node_facade_sub(node->function.function_impl, json, "implementation");
}

void FUNCTION_CALL_to_json(SyntaxNode *node, JSONValue *json)
{
    node_facade_sub(node->call.function, json, "function");
    json_set(json, "discard_result", json_bool(node->call.discard_result));
    node_chain_to_array(node->call.arguments, json, "arguments");
}

void FUNCTION_IMPL_to_json(SyntaxNode *node, JSONValue *json)
{
    node_chain_to_array(node->function_impl.statements, json, "statements");
}

void IF_to_json(SyntaxNode *node, JSONValue *json)
{
    node_facade_sub(node->if_statement.condition, json, "condition");
    node_chain_to_array(node->if_statement.if_true, json, "if_true");
    node_chain_to_array(node->if_statement.if_false, json, "if_false");
}

void INTEGER_to_json(SyntaxNode *, JSONValue *)
{
}

void IMPORT_to_json(SyntaxNode *node, JSONValue *json)
{
    node_chain_to_array(node->import.modules, json, "modules");
}

void LABEL_to_json(SyntaxNode *, JSONValue *)
{
}

void LOOP_to_json(SyntaxNode *node, JSONValue *json)
{
    BLOCK_to_json(node, json);
}

void MACRO_to_json(SyntaxNode *, JSONValue *)
{
}

void MODULE_to_json(SyntaxNode *node, JSONValue *json)
{
    BLOCK_to_json(node, json);
}

void NAME_to_json(SyntaxNode *, JSONValue *)
{
}

void NATIVE_FUNCTION_to_json(SyntaxNode *, JSONValue *)
{
}

void PARAMETER_to_json(SyntaxNode *node, JSONValue *json)
{
    node_facade_sub(node->parameter.parameter_type, json, "parameter_type");
}

void PROGRAM_to_json(SyntaxNode *node, JSONValue *json)
{
    node_chain_to_array(node->program.imports, json, "imports");
    node_chain_to_array(node->program.modules, json, "modules");
}

void RETURN_to_json(SyntaxNode *node, JSONValue *json)
{
    node_facade_sub(node->return_stmt.expression, json, "expression");
}

void STRING_to_json(SyntaxNode *, JSONValue *)
{
}

void STRUCT_to_json(SyntaxNode *node, JSONValue *json)
{
    node_chain_to_array(node->struct_def.components, json, "components");
}

void TYPE_to_json(SyntaxNode *node, JSONValue *json)
{
    json_set(json, "type_descr", type_descr_to_json(&node->type_descr));
}

void TYPE_COMPONENT_to_json(SyntaxNode *node, JSONValue *json)
{
    PARAMETER_to_json(node, json);
}

void UNARYEXPRESSION_to_json(SyntaxNode *node, JSONValue *json)
{
    json_set_cstr(json, "operator", Operator_name(node->unary_expr.operator));
    node_facade_sub(node->unary_expr.operand, json, "operand");
}

void VARIABLE_to_json(SyntaxNode *node, JSONValue *json)
{
    if (node->variable.subscript) {
        JSONValue sub = json_array();
        for (SyntaxNode *n = node->variable.subscript; n; n = n->next) {
            json_append(&sub, node_facade(n));
        }
        json_set(json, "subscript", sub);
    }
}

void VARIABLE_DECL_to_json(SyntaxNode *node, JSONValue *json)
{
    json_set(json, "is_const", json_bool(node->variable_decl.is_const));
    node_facade_sub(node->variable_decl.variable, json, "variable");
    if (node->variable_decl.var_type) {
        node_facade_sub(node->variable_decl.var_type, json, "variable_type");
    }
    if (node->variable_decl.init_expr) {
        node_facade_sub(node->variable_decl.init_expr, json, "expression");
    }
}

void VARIANT_to_json(SyntaxNode *node, JSONValue *json)
{
    node_facade_sub(node->variant_def.underlying_type, json, "underlying_type");
    node_chain_to_array(node->variant_def.options, json, "values");
}

void VARIANT_OPTION_to_json(SyntaxNode *node, JSONValue *json)
{
    node_facade_sub(node->variant_option.underlying_value, json, "underlying_value");
    node_facade_sub(node->variant_option.payload_type, json, "payload");
}

void WHILE_to_json(SyntaxNode *node, JSONValue *json)
{
    node_facade_sub(node->while_statement.condition, json, "condition");
    node_chain_to_array(node->while_statement.statement, json, "statement");
}

JSONValue syntax_node_to_json(SyntaxNode *node)
{
    JSONValue json = json_object();
    json_set_string(&json, "name", node->name);
    json_set_cstr(&json, "type", SyntaxNodeType_name(node->type));
    json_set_int(&json, "index", (int) node->index);
    json_set(&json, "token", token_to_json(node->token));
    switch (node->type) {
#undef SYNTAXNODETYPE
#define SYNTAXNODETYPE(type)         \
    case SNT_##type:                 \
        type##_to_json(node, &json); \
        break;
        SYNTAXNODETYPES(SYNTAXNODETYPE)
#undef SYNTAXNODETYPE
    default:
        UNREACHABLE();
    }
    return json;
}
