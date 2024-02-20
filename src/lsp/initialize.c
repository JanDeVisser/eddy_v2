/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */
 
// clang-format off 

#include <lsp/initialize.h>

OPTIONAL_JSON_IMPL(TraceValue);


StringView TraceValue_to_string(TraceValue value)
{
    switch(value) {
    case TraceValueOff: return sv_from("off");
    case TraceValueMessages: return sv_from("messages");
    case TraceValueVerbose: return sv_from("verbose");
    default: UNREACHABLE();
    }
}

TraceValue TraceValue_parse(StringView s)
{
    if (sv_eq_cstr(s, "off")) return TraceValueOff;
    if (sv_eq_cstr(s, "messages")) return TraceValueMessages;
    if (sv_eq_cstr(s, "verbose")) return TraceValueVerbose;
    UNREACHABLE();
}

TraceValue TraceValue_decode(OptionalJSONValue json)
{
    assert(json.has_value);
    assert(json.value.type == JSON_TYPE_STRING);
    return TraceValue_parse(json.value.string);
}

OptionalJSONValue TraceValue_encode(TraceValue value)
{
    RETURN_VALUE(JSONValue, json_string(TraceValue_to_string(value)));
}

DA_IMPL(TokenFormat);
DA_JSON_IMPL(TokenFormat, TokenFormats, elements);


StringView TokenFormat_to_string(TokenFormat value)
{
    switch(value) {
    case TokenFormatRelative: return sv_from("relative");
    default: UNREACHABLE();
    }
}

TokenFormat TokenFormat_parse(StringView s)
{
    if (sv_eq_cstr(s, "relative")) return TokenFormatRelative;
    UNREACHABLE();
}

TokenFormat TokenFormat_decode(OptionalJSONValue json)
{
    assert(json.has_value);
    assert(json.value.type == JSON_TYPE_STRING);
    return TokenFormat_parse(json.value.string);
}

OptionalJSONValue TokenFormat_encode(TokenFormat value)
{
    RETURN_VALUE(JSONValue, json_string(TokenFormat_to_string(value)));
}

DA_IMPL(SemanticTokenTypes);
DA_JSON_IMPL(SemanticTokenTypes, SemanticTokenTypess, elements);


StringView SemanticTokenTypes_to_string(SemanticTokenTypes value)
{
    switch(value) {
    case SemanticTokenTypesNamespace: return sv_from("namespace");
    case SemanticTokenTypesType: return sv_from("type");
    case SemanticTokenTypesClass: return sv_from("class");
    case SemanticTokenTypesEnum: return sv_from("enum");
    case SemanticTokenTypesInterface: return sv_from("interface");
    case SemanticTokenTypesStruct: return sv_from("struct");
    case SemanticTokenTypesTypeParameter: return sv_from("typeParameter");
    case SemanticTokenTypesParameter: return sv_from("parameter");
    case SemanticTokenTypesVariable: return sv_from("variable");
    case SemanticTokenTypesProperty: return sv_from("property");
    case SemanticTokenTypesEnumMember: return sv_from("enumMember");
    case SemanticTokenTypesEvent: return sv_from("event");
    case SemanticTokenTypesFunction: return sv_from("function");
    case SemanticTokenTypesMethod: return sv_from("method");
    case SemanticTokenTypesMacro: return sv_from("macro");
    case SemanticTokenTypesKeyword: return sv_from("keyword");
    case SemanticTokenTypesModifier: return sv_from("modifier");
    case SemanticTokenTypesComment: return sv_from("comment");
    case SemanticTokenTypesString: return sv_from("string");
    case SemanticTokenTypesNumber: return sv_from("number");
    case SemanticTokenTypesRegexp: return sv_from("regexp");
    case SemanticTokenTypesOperator: return sv_from("operator");
    case SemanticTokenTypesDecorator: return sv_from("decorator");
    case SemanticTokenTypesUnknown: return sv_from("unknown");
    default: UNREACHABLE();
    }
}

SemanticTokenTypes SemanticTokenTypes_parse(StringView s)
{
    if (sv_eq_cstr(s, "namespace")) return SemanticTokenTypesNamespace;
    if (sv_eq_cstr(s, "type")) return SemanticTokenTypesType;
    if (sv_eq_cstr(s, "class")) return SemanticTokenTypesClass;
    if (sv_eq_cstr(s, "enum")) return SemanticTokenTypesEnum;
    if (sv_eq_cstr(s, "interface")) return SemanticTokenTypesInterface;
    if (sv_eq_cstr(s, "struct")) return SemanticTokenTypesStruct;
    if (sv_eq_cstr(s, "typeParameter")) return SemanticTokenTypesTypeParameter;
    if (sv_eq_cstr(s, "parameter")) return SemanticTokenTypesParameter;
    if (sv_eq_cstr(s, "variable")) return SemanticTokenTypesVariable;
    if (sv_eq_cstr(s, "property")) return SemanticTokenTypesProperty;
    if (sv_eq_cstr(s, "enumMember")) return SemanticTokenTypesEnumMember;
    if (sv_eq_cstr(s, "event")) return SemanticTokenTypesEvent;
    if (sv_eq_cstr(s, "function")) return SemanticTokenTypesFunction;
    if (sv_eq_cstr(s, "method")) return SemanticTokenTypesMethod;
    if (sv_eq_cstr(s, "macro")) return SemanticTokenTypesMacro;
    if (sv_eq_cstr(s, "keyword")) return SemanticTokenTypesKeyword;
    if (sv_eq_cstr(s, "modifier")) return SemanticTokenTypesModifier;
    if (sv_eq_cstr(s, "comment")) return SemanticTokenTypesComment;
    if (sv_eq_cstr(s, "string")) return SemanticTokenTypesString;
    if (sv_eq_cstr(s, "number")) return SemanticTokenTypesNumber;
    if (sv_eq_cstr(s, "regexp")) return SemanticTokenTypesRegexp;
    if (sv_eq_cstr(s, "operator")) return SemanticTokenTypesOperator;
    if (sv_eq_cstr(s, "decorator")) return SemanticTokenTypesDecorator;
    if (sv_eq_cstr(s, "unknown")) return SemanticTokenTypesUnknown;
    UNREACHABLE();
}

SemanticTokenTypes SemanticTokenTypes_decode(OptionalJSONValue json)
{
    assert(json.has_value);
    assert(json.value.type == JSON_TYPE_STRING);
    return SemanticTokenTypes_parse(json.value.string);
}

OptionalJSONValue SemanticTokenTypes_encode(SemanticTokenTypes value)
{
    RETURN_VALUE(JSONValue, json_string(SemanticTokenTypes_to_string(value)));
}


StringView SemanticTokenModifiers_to_string(SemanticTokenModifiers value)
{
    switch(value) {
    case SemanticTokenModifiersDeclaration: return sv_from("declaration");
    case SemanticTokenModifiersDefinition: return sv_from("definition");
    case SemanticTokenModifiersReadonly: return sv_from("readonly");
    case SemanticTokenModifiersStatic: return sv_from("static");
    case SemanticTokenModifiersDeprecated: return sv_from("deprecated");
    case SemanticTokenModifiersAbstract: return sv_from("abstract");
    case SemanticTokenModifiersAsync: return sv_from("async");
    case SemanticTokenModifiersModification: return sv_from("modification");
    case SemanticTokenModifiersDocumentation: return sv_from("documentation");
    case SemanticTokenModifiersDefaultLibrary: return sv_from("defaultLibrary");
    case SemanticTokenModifiersUnknown: return sv_from("unknown");
    default: UNREACHABLE();
    }
}

SemanticTokenModifiers SemanticTokenModifiers_parse(StringView s)
{
    if (sv_eq_cstr(s, "declaration")) return SemanticTokenModifiersDeclaration;
    if (sv_eq_cstr(s, "definition")) return SemanticTokenModifiersDefinition;
    if (sv_eq_cstr(s, "readonly")) return SemanticTokenModifiersReadonly;
    if (sv_eq_cstr(s, "static")) return SemanticTokenModifiersStatic;
    if (sv_eq_cstr(s, "deprecated")) return SemanticTokenModifiersDeprecated;
    if (sv_eq_cstr(s, "abstract")) return SemanticTokenModifiersAbstract;
    if (sv_eq_cstr(s, "async")) return SemanticTokenModifiersAsync;
    if (sv_eq_cstr(s, "modification")) return SemanticTokenModifiersModification;
    if (sv_eq_cstr(s, "documentation")) return SemanticTokenModifiersDocumentation;
    if (sv_eq_cstr(s, "defaultLibrary")) return SemanticTokenModifiersDefaultLibrary;
    if (sv_eq_cstr(s, "unknown")) return SemanticTokenModifiersUnknown;
    UNREACHABLE();
}

SemanticTokenModifiers SemanticTokenModifiers_decode(OptionalJSONValue json)
{
    assert(json.has_value);
    assert(json.value.type == JSON_TYPE_STRING);
    return SemanticTokenModifiers_parse(json.value.string);
}

OptionalJSONValue SemanticTokenModifiers_encode(SemanticTokenModifiers value)
{
    RETURN_VALUE(JSONValue, json_string(SemanticTokenModifiers_to_string(value)));
}

OPTIONAL_JSON_IMPL(TextDocumentSyncKind);

TextDocumentSyncKind TextDocumentSyncKind_decode(OptionalJSONValue json)
{
    assert(json.has_value);
    assert(json.value.type == JSON_TYPE_INT);
    return (TextDocumentSyncKind) json_int_value(json.value);
}

OptionalJSONValue TextDocumentSyncKind_encode(TextDocumentSyncKind value)
{
    RETURN_VALUE(JSONValue, json_int((int) value));
}

OPTIONAL_JSON_ENCODE_IMPL(TextDocumentSyncClientCapabilities);

OptionalJSONValue TextDocumentSyncClientCapabilities_encode(TextDocumentSyncClientCapabilities value)
{
    JSONValue v4 = json_object();
    json_optional_set(&v4, "dynamicRegistration", OptionalBool_encode(value.dynamicRegistration));
    json_optional_set(&v4, "willSave", OptionalBool_encode(value.willSave));
    json_optional_set(&v4, "willSaveWaitUntil", OptionalBool_encode(value.willSaveWaitUntil));
    json_optional_set(&v4, "didSave", OptionalBool_encode(value.didSave));
    RETURN_VALUE(JSONValue, v4);
}

OPTIONAL_JSON_ENCODE_IMPL(SemanticTokensClientCapabilities);

OptionalJSONValue SemanticTokensClientCapabilities_encode(SemanticTokensClientCapabilities value)
{
    JSONValue v4 = json_object();
    json_optional_set(&v4, "dynamicRegistration", OptionalBool_encode(value.dynamicRegistration));
    {
        JSONValue v8 = json_object();
        if (value.requests.range.has_value) {
            JSONValue v12 = {0};
            switch (value.requests.range.tag) {
            case 0:
                v12 = json_bool(value.requests.range._0);
                break;
            case 1:
                v12 = json_object();
                break;
            default:
                UNREACHABLE();
            }
            json_set(&v8, "range", v12);
        }
        if (value.requests.full.has_value) {
            JSONValue v12 = {0};
            switch (value.requests.full.tag) {
            case 0:
                v12 = json_bool(value.requests.full._0);
                break;
            case 1:
                v12 = json_object();
                json_optional_set(&v12, "delta", OptionalBool_encode(value.requests.full._1.delta));
                break;
            default:
                UNREACHABLE();
            }
            json_set(&v8, "full", v12);
        }
        json_set(&v4, "requests", v8);
    }
    json_optional_set(&v4, "tokenTypes", StringList_encode(value.tokenTypes));
    json_optional_set(&v4, "tokenModifiers", StringList_encode(value.tokenModifiers));
    json_optional_set(&v4, "formats", TokenFormats_encode(value.formats));
    json_optional_set(&v4, "overlappingTokenSupport", OptionalBool_encode(value.overlappingTokenSupport));
    json_optional_set(&v4, "multilineTokenSupport", OptionalBool_encode(value.multilineTokenSupport));
    json_optional_set(&v4, "serverCancelSupport", OptionalBool_encode(value.serverCancelSupport));
    RETURN_VALUE(JSONValue, v4);
}

OPTIONAL_JSON_ENCODE_IMPL(TextDocumentClientCapabilities);

OptionalJSONValue TextDocumentClientCapabilities_encode(TextDocumentClientCapabilities value)
{
    JSONValue v4 = json_object();
    json_optional_set(&v4, "synchronization", OptionalTextDocumentSyncClientCapabilities_encode(value.synchronization));
    json_optional_set(&v4, "semanticTokens", OptionalSemanticTokensClientCapabilities_encode(value.semanticTokens));
    RETURN_VALUE(JSONValue, v4);
}

OptionalJSONValue ClientCapabilities_encode(ClientCapabilities value)
{
    JSONValue v4 = json_object();
    json_optional_set(&v4, "textDocument", OptionalTextDocumentClientCapabilities_encode(value.textDocument));
    RETURN_VALUE(JSONValue, v4);
}

DA_IMPL(WorkspaceFolder);
DA_JSON_ENCODE_IMPL(WorkspaceFolder, WorkspaceFolders, elements);
DA_JSON_DECODE_IMPL(WorkspaceFolder, WorkspaceFolders, elements);

WorkspaceFolder WorkspaceFolder_decode(OptionalJSONValue v4)
{
    assert(v4.has_value);
    assert(v4.value.type == JSON_TYPE_OBJECT);
    WorkspaceFolder value = {0};
    {
        OptionalJSONValue v8 = json_get(&v4.value, "uri");
        value.uri = StringView_decode(v8);
    }
    {
        OptionalJSONValue v8 = json_get(&v4.value, "name");
        value.name = StringView_decode(v8);
    }
    return value;
}

OptionalJSONValue WorkspaceFolder_encode(WorkspaceFolder value)
{
    JSONValue v4 = json_object();
    json_optional_set(&v4, "uri", StringView_encode(value.uri));
    json_optional_set(&v4, "name", StringView_encode(value.name));
    RETURN_VALUE(JSONValue, v4);
}

OptionalJSONValue InitializeParams_encode(InitializeParams value)
{
    JSONValue v4 = json_object();
    assert(value.processId.has_value);
    {
        JSONValue v8 = {0};
        switch (value.processId.tag) {
        case 0:
            v8 = json_int(value.processId._0);
            break;
        case 1:
            v8 = json_null();
            break;
        default:
            UNREACHABLE();
        }
        json_set(&v4, "processId", v8);
    }
    if (value.clientInfo.has_value) {
        JSONValue v8 = json_object();
        json_optional_set(&v8, "name", StringView_encode(value.clientInfo.name));
        json_optional_set(&v8, "version", OptionalStringView_encode(value.clientInfo.version));
        json_set(&v4, "clientInfo", v8);
    }
    json_optional_set(&v4, "locale", OptionalStringView_encode(value.locale));
    json_optional_set(&v4, "rootPath", OptionalStringView_encode(value.rootPath));
    json_optional_set(&v4, "rootUri", OptionalStringView_encode(value.rootUri));
    json_optional_set(&v4, "initializationOptions", OptionalJSONValue_encode(value.initializationOptions));
    json_optional_set(&v4, "capabilities", ClientCapabilities_encode(value.capabilities));
    json_optional_set(&v4, "trace", OptionalTraceValue_encode(value.trace));
    if (value.workspaceFolders.has_value) {
        JSONValue v8 = {0};
        switch (value.workspaceFolders.tag) {
        case 0:
            v8 = WorkspaceFolders_encode(value.workspaceFolders._0).value;
            break;
        case 1:
            v8 = json_null();
            break;
        default:
            UNREACHABLE();
        }
        json_set(&v4, "workspaceFolders", v8);
    }
    RETURN_VALUE(JSONValue, v4);
}

SemanticTokensLegend SemanticTokensLegend_decode(OptionalJSONValue v4)
{
    assert(v4.has_value);
    assert(v4.value.type == JSON_TYPE_OBJECT);
    SemanticTokensLegend value = {0};
    {
        OptionalJSONValue v8 = json_get(&v4.value, "tokenTypes");
        value.tokenTypes = StringList_decode(v8);
    }
    {
        OptionalJSONValue v8 = json_get(&v4.value, "tokenModifiers");
        value.tokenModifiers = StringList_decode(v8);
    }
    return value;
}

OptionalJSONValue SemanticTokensLegend_encode(SemanticTokensLegend value)
{
    JSONValue v4 = json_object();
    json_optional_set(&v4, "tokenTypes", StringList_encode(value.tokenTypes));
    json_optional_set(&v4, "tokenModifiers", StringList_encode(value.tokenModifiers));
    RETURN_VALUE(JSONValue, v4);
}

OPTIONAL_JSON_DECODE_IMPL(SemanticTokensOptions);

SemanticTokensOptions SemanticTokensOptions_decode(OptionalJSONValue v4)
{
    assert(v4.has_value);
    assert(v4.value.type == JSON_TYPE_OBJECT);
    SemanticTokensOptions value = {0};
    {
        OptionalJSONValue v8 = json_get(&v4.value, "legend");
        value.legend = SemanticTokensLegend_decode(v8);
    }
    {
        OptionalJSONValue v8 = json_get(&v4.value, "range");
        if (v8.has_value) {
            value.range.has_value = true;
            if (v8.value.type == JSON_TYPE_BOOLEAN) {
                value.range.tag = 0;
                value.range._0 = v8.value.boolean;
            }
            if (v8.value.type == JSON_TYPE_OBJECT && v8.value.object.size == 0) {
                value.range.tag = 1;
                value.range._1 = (Empty) {};
            }
        }
    }
    {
        OptionalJSONValue v8 = json_get(&v4.value, "full");
        if (v8.has_value) {
            value.full.has_value = true;
            if (v8.value.type == JSON_TYPE_BOOLEAN) {
                value.full.tag = 0;
                value.full._0 = v8.value.boolean;
            }
            if (v8.value.type == JSON_TYPE_OBJECT) {
                value.full.tag = 1;
                {
                    OptionalJSONValue v20 = json_get(&v8.value, "delta");
                    value.full._1.delta = OptionalBool_decode(v20);
                }
            }
        }
    }
    return value;
}

OPTIONAL_JSON_DECODE_IMPL(TextDocumentSyncOptions);

TextDocumentSyncOptions TextDocumentSyncOptions_decode(OptionalJSONValue v4)
{
    assert(v4.has_value);
    assert(v4.value.type == JSON_TYPE_OBJECT);
    TextDocumentSyncOptions value = {0};
    {
        OptionalJSONValue v8 = json_get(&v4.value, "openClose");
        value.openClose = OptionalBool_decode(v8);
    }
    {
        OptionalJSONValue v8 = json_get(&v4.value, "change");
        value.change = OptionalTextDocumentSyncKind_decode(v8);
    }
    return value;
}

ServerCapabilities ServerCapabilities_decode(OptionalJSONValue v4)
{
    assert(v4.has_value);
    assert(v4.value.type == JSON_TYPE_OBJECT);
    ServerCapabilities value = {0};
    {
        OptionalJSONValue v8 = json_get(&v4.value, "textDocumentSync");
        if (v8.has_value) {
            value.textDocumentSync.has_value = true;
            if (v8.value.type == JSON_TYPE_OBJECT) {
                value.textDocumentSync.tag = 0;
                value.textDocumentSync._0 = TextDocumentSyncOptions_decode(v8);
            }
            if (v8.value.type == JSON_TYPE_INT) {
                value.textDocumentSync.tag = 1;
                value.textDocumentSync._1 = TextDocumentSyncKind_decode(v8);
            }
        }
    }
    {
        OptionalJSONValue v8 = json_get(&v4.value, "semanticTokensProvider");
        value.semanticTokensProvider = OptionalSemanticTokensOptions_decode(v8);
    }
    return value;
}

InitializeResult InitializeResult_decode(OptionalJSONValue v4)
{
    assert(v4.has_value);
    assert(v4.value.type == JSON_TYPE_OBJECT);
    InitializeResult value = {0};
    {
        OptionalJSONValue v8 = json_get(&v4.value, "capabilities");
        value.capabilities = ServerCapabilities_decode(v8);
    }
    {
        OptionalJSONValue v8 = json_get(&v4.value, "serverInfo");
        if (v8.has_value) {
            value.serverInfo.has_value = true;
            {
                OptionalJSONValue v16 = json_get(&v8.value, "name");
                value.serverInfo.name = StringView_decode(v16);
            }
            {
                OptionalJSONValue v16 = json_get(&v8.value, "version");
                value.serverInfo.version = OptionalStringView_decode(v16);
            }
        }
    }
    return value;
}

// clang-format on

