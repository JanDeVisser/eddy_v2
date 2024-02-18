/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */
 
// clang-format off 

#include <lsp/synchronization.h>

OptionalJSONValue DidOpenTextDocumentParams_encode(DidOpenTextDocumentParams value)
{
    JSONValue v4 = json_object();
    json_optional_set(&v4, "textDocument", TextDocumentItem_encode(value.textDocument));
    RETURN_VALUE(JSONValue, v4);
}

OptionalJSONValue DidSaveTextDocumentParams_encode(DidSaveTextDocumentParams value)
{
    JSONValue v4 = json_object();
    json_optional_set(&v4, "textDocument", TextDocumentIdentifier_encode(value.textDocument));
    json_optional_set(&v4, "text", OptionalStringView_encode(value.text));
    RETURN_VALUE(JSONValue, v4);
}

OptionalJSONValue DidCloseTextDocumentParams_encode(DidCloseTextDocumentParams value)
{
    JSONValue v4 = json_object();
    json_optional_set(&v4, "textDocument", TextDocumentIdentifier_encode(value.textDocument));
    RETURN_VALUE(JSONValue, v4);
}

DA_IMPL(TextDocumentContentChangeEvent);
DA_JSON_ENCODE_IMPL(TextDocumentContentChangeEvent, TextDocumentContentChangeEvents, elements);
DA_JSON_DECODE_IMPL(TextDocumentContentChangeEvent, TextDocumentContentChangeEvents, elements);

TextDocumentContentChangeEvent TextDocumentContentChangeEvent_decode(OptionalJSONValue v4)
{
    assert(v4.has_value);
    assert(v4.value.type == JSON_TYPE_OBJECT);
    TextDocumentContentChangeEvent value = {0};
    {
        OptionalJSONValue v8 = json_get(&v4.value, "range");
        value.range = Range_decode(v8);
    }
    {
        OptionalJSONValue v8 = json_get(&v4.value, "rangeLength");
        value.rangeLength = OptionalUInt_decode(v8);
    }
    {
        OptionalJSONValue v8 = json_get(&v4.value, "text");
        value.text = StringView_decode(v8);
    }
    return value;
}

OptionalJSONValue TextDocumentContentChangeEvent_encode(TextDocumentContentChangeEvent value)
{
    JSONValue v4 = json_object();
    json_optional_set(&v4, "range", Range_encode(value.range));
    json_optional_set(&v4, "rangeLength", OptionalUInt_encode(value.rangeLength));
    json_optional_set(&v4, "text", StringView_encode(value.text));
    RETURN_VALUE(JSONValue, v4);
}

OptionalJSONValue DidChangeTextDocumentParams_encode(DidChangeTextDocumentParams value)
{
    JSONValue v4 = json_object();
    json_optional_set(&v4, "textDocument", VersionedTextDocumentIdentifier_encode(value.textDocument));
    json_optional_set(&v4, "contentChanges", TextDocumentContentChangeEvents_encode(value.contentChanges));
    RETURN_VALUE(JSONValue, v4);
}

// clang-format on

