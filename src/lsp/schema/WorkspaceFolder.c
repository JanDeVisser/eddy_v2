#include "json.h"
/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/WorkspaceFolder.h>

DA_IMPL(WorkspaceFolder)

OptionalJSONValue WorkspaceFolders_encode(WorkspaceFolders value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, WorkspaceFolder_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalWorkspaceFolders WorkspaceFolders_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(WorkspaceFolders);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    WorkspaceFolders ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue       elem = json_at(&json.value, ix);
        OptionalWorkspaceFolder val = WorkspaceFolder_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(WorkspaceFolders);
        }
        da_append_WorkspaceFolder(&ret, val.value);
    }
    RETURN_VALUE(WorkspaceFolders, ret);
}

OptionalWorkspaceFolder WorkspaceFolder_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(WorkspaceFolder);
    }
    WorkspaceFolder value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "uri");
        value.uri = FORWARD_OPTIONAL(URI, WorkspaceFolder, URI_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "name");
        value.name = FORWARD_OPTIONAL(StringView, WorkspaceFolder, StringView_decode(v0));
    }
    RETURN_VALUE(WorkspaceFolder, value);
}

OptionalJSONValue WorkspaceFolder_encode(WorkspaceFolder value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = URI_encode(value.uri);
        json_optional_set(&v1, "uri", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = StringView_encode(value.name);
        json_optional_set(&v1, "name", _encoded_maybe);
    }
    RETURN_VALUE(JSONValue, v1);
}
