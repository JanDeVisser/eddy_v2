/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/InitializeParams.h>

DA_IMPL(InitializeParams)

OptionalJSONValue InitializeParamss_encode(InitializeParamss value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, InitializeParams_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalInitializeParamss InitializeParamss_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(InitializeParamss);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    InitializeParamss ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue        elem = json_at(&json.value, ix);
        OptionalInitializeParams val = InitializeParams_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(InitializeParamss);
        }
        da_append_InitializeParams(&ret, val.value);
    }
    RETURN_VALUE(InitializeParamss, ret);
}

OptionalInitializeParams InitializeParams_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(InitializeParams);
    }
    InitializeParams value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "processId");
        assert(v0.has_value);
        value.processId.has_value = true;
        while (true) {
            {
                OptionalInt decoded = Int_decode(v0);
                if (decoded.has_value) {
                    value.processId.tag = 0;
                    value.processId._0 = decoded.value;
                    break;
                }
            }
            {
                OptionalNull decoded = Null_decode(v0);
                if (decoded.has_value) {
                    value.processId.tag = 1;
                    value.processId._1 = decoded.value;
                    break;
                }
            }
            RETURN_EMPTY(InitializeParams);
        }
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "clientInfo");
        if (v0.has_value) {
            value.clientInfo.has_value = true;
            {
                OptionalJSONValue v1 = json_get(&v0.value, "name");
                value.clientInfo.name = FORWARD_OPTIONAL(StringView, InitializeParams, StringView_decode(v1));
            }

            {
                OptionalJSONValue v1 = json_get(&v0.value, "version");
                value.clientInfo.version = StringView_decode(v1);
            }
        }
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "locale");
        value.locale = StringView_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "rootPath");
        if (v0.has_value) {
            value.rootPath.has_value = true;
            while (true) {
                {
                    OptionalStringView decoded = StringView_decode(v0);
                    if (decoded.has_value) {
                        value.rootPath.tag = 0;
                        value.rootPath._0 = decoded.value;
                        break;
                    }
                }
                {
                    OptionalNull decoded = Null_decode(v0);
                    if (decoded.has_value) {
                        value.rootPath.tag = 1;
                        value.rootPath._1 = decoded.value;
                        break;
                    }
                }
                RETURN_EMPTY(InitializeParams);
            }
        }
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "rootUri");
        assert(v0.has_value);
        value.rootUri.has_value = true;
        while (true) {
            {
                OptionalDocumentUri decoded = DocumentUri_decode(v0);
                if (decoded.has_value) {
                    value.rootUri.tag = 0;
                    value.rootUri._0 = decoded.value;
                    break;
                }
            }
            {
                OptionalNull decoded = Null_decode(v0);
                if (decoded.has_value) {
                    value.rootUri.tag = 1;
                    value.rootUri._1 = decoded.value;
                    break;
                }
            }
            RETURN_EMPTY(InitializeParams);
        }
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "initializationOptions");
        value.initializationOptions = JSONValue_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "capabilities");
        value.capabilities = FORWARD_OPTIONAL(ClientCapabilities, InitializeParams, ClientCapabilities_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "trace");
        value.trace = TraceValue_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "workspaceFolders");
        if (v0.has_value) {
            value.workspaceFolders.has_value = true;
            while (true) {
                {
                    OptionalWorkspaceFolders decoded = WorkspaceFolders_decode(v0);
                    if (decoded.has_value) {
                        value.workspaceFolders.tag = 0;
                        value.workspaceFolders._0 = decoded.value;
                        break;
                    }
                }
                {
                    OptionalNull decoded = Null_decode(v0);
                    if (decoded.has_value) {
                        value.workspaceFolders.tag = 1;
                        value.workspaceFolders._1 = decoded.value;
                        break;
                    }
                }
                RETURN_EMPTY(InitializeParams);
            }
        }
    }
    RETURN_VALUE(InitializeParams, value);
}

OptionalJSONValue InitializeParams_encode(InitializeParams value)
{
    JSONValue v1 = json_object();
    assert(value.processId.has_value);
    {
        JSONValue v2 = { 0 };
        switch (value.processId.tag) {
        case 0:
            v2 = MUST_OPTIONAL(JSONValue, Int_encode(value.processId._0));

            break;
        case 1:
            v2 = MUST_OPTIONAL(JSONValue, Null_encode(value.processId._1));

            break;
        default:
            UNREACHABLE();
        }

        json_set(&v1, "processId", v2);
    }

    if (value.clientInfo.has_value) {
        JSONValue v2 = json_object();
        {
            OptionalJSONValue _encoded_maybe = { 0 };
            _encoded_maybe = StringView_encode(value.clientInfo.name);
            json_optional_set(&v2, "name", _encoded_maybe);
        }
        {
            OptionalJSONValue _encoded_maybe = { 0 };
            if (value.clientInfo.version.has_value) {
                _encoded_maybe = StringView_encode(value.clientInfo.version.value);
            }
            json_optional_set(&v2, "version", _encoded_maybe);
        }
        json_set(&v1, "clientInfo", v2);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.locale.has_value) {
            _encoded_maybe = StringView_encode(value.locale.value);
        }
        json_optional_set(&v1, "locale", _encoded_maybe);
    }
    if (value.rootPath.has_value) {
        JSONValue v2 = { 0 };
        switch (value.rootPath.tag) {
        case 0:
            v2 = MUST_OPTIONAL(JSONValue, StringView_encode(value.rootPath._0));

            break;
        case 1:
            v2 = MUST_OPTIONAL(JSONValue, Null_encode(value.rootPath._1));

            break;
        default:
            UNREACHABLE();
        }

        json_set(&v1, "rootPath", v2);
    }

    assert(value.rootUri.has_value);
    {
        JSONValue v2 = { 0 };
        switch (value.rootUri.tag) {
        case 0:
            v2 = MUST_OPTIONAL(JSONValue, DocumentUri_encode(value.rootUri._0));

            break;
        case 1:
            v2 = MUST_OPTIONAL(JSONValue, Null_encode(value.rootUri._1));

            break;
        default:
            UNREACHABLE();
        }

        json_set(&v1, "rootUri", v2);
    }

    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.initializationOptions.has_value) {
            _encoded_maybe = JSONValue_encode(value.initializationOptions.value);
        }
        json_optional_set(&v1, "initializationOptions", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = ClientCapabilities_encode(value.capabilities);
        json_optional_set(&v1, "capabilities", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.trace.has_value) {
            _encoded_maybe = TraceValue_encode(value.trace.value);
        }
        json_optional_set(&v1, "trace", _encoded_maybe);
    }
    if (value.workspaceFolders.has_value) {
        JSONValue v2 = { 0 };
        switch (value.workspaceFolders.tag) {
        case 0:
            v2 = MUST_OPTIONAL(JSONValue, WorkspaceFolders_encode(value.workspaceFolders._0));

            break;
        case 1:
            v2 = MUST_OPTIONAL(JSONValue, Null_encode(value.workspaceFolders._1));

            break;
        default:
            UNREACHABLE();
        }

        json_set(&v1, "workspaceFolders", v2);
    }

    RETURN_VALUE(JSONValue, v1);
}
