/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */
 
// clang-format off 

#include <lsp/base.h>

OPTIONAL_JSON_ENCODE_IMPL(Position);
OPTIONAL_JSON_DECODE_IMPL(Position);

DA_IMPL(Position);
DA_JSON_ENCODE_IMPL(Position, Positions, elements);
DA_JSON_DECODE_IMPL(Position, Positions, elements);
OPTIONAL_JSON_ENCODE_IMPL(Positions);
OPTIONAL_JSON_DECODE_IMPL(Positions);

Position Position_decode(OptionalJSONValue v4)
{
    assert(v4.has_value);
    assert(v4.value.type == JSON_TYPE_OBJECT);
    Position value = {0};
    {
        OptionalJSONValue v8 = json_get(&v4.value, "line");
        value.line = UInt_decode(v8);
    }
    {
        OptionalJSONValue v8 = json_get(&v4.value, "character");
        value.character = UInt_decode(v8);
    }
    return value;
}

OptionalJSONValue Position_encode(Position value)
{
    JSONValue v4 = json_object();
    json_optional_set(&v4, "line", UInt_encode(value.line));
    json_optional_set(&v4, "character", UInt_encode(value.character));
    RETURN_VALUE(JSONValue, v4);
}

OPTIONAL_JSON_ENCODE_IMPL(Range);
OPTIONAL_JSON_DECODE_IMPL(Range);

DA_IMPL(Range);
DA_JSON_ENCODE_IMPL(Range, Ranges, elements);
DA_JSON_DECODE_IMPL(Range, Ranges, elements);
OPTIONAL_JSON_ENCODE_IMPL(Ranges);
OPTIONAL_JSON_DECODE_IMPL(Ranges);

Range Range_decode(OptionalJSONValue v4)
{
    assert(v4.has_value);
    assert(v4.value.type == JSON_TYPE_OBJECT);
    Range value = {0};
    {
        OptionalJSONValue v8 = json_get(&v4.value, "start");
        value.start = Position_decode(v8);
    }
    {
        OptionalJSONValue v8 = json_get(&v4.value, "end");
        value.end = Position_decode(v8);
    }
    return value;
}

OptionalJSONValue Range_encode(Range value)
{
    JSONValue v4 = json_object();
    json_optional_set(&v4, "start", Position_encode(value.start));
    json_optional_set(&v4, "end", Position_encode(value.end));
    RETURN_VALUE(JSONValue, v4);
}

OPTIONAL_JSON_ENCODE_IMPL(TextDocumentItem);
OPTIONAL_JSON_DECODE_IMPL(TextDocumentItem);

DA_IMPL(TextDocumentItem);
DA_JSON_ENCODE_IMPL(TextDocumentItem, TextDocumentItems, elements);
DA_JSON_DECODE_IMPL(TextDocumentItem, TextDocumentItems, elements);
OPTIONAL_JSON_ENCODE_IMPL(TextDocumentItems);
OPTIONAL_JSON_DECODE_IMPL(TextDocumentItems);

TextDocumentItem TextDocumentItem_decode(OptionalJSONValue v4)
{
    assert(v4.has_value);
    assert(v4.value.type == JSON_TYPE_OBJECT);
    TextDocumentItem value = {0};
    {
        OptionalJSONValue v8 = json_get(&v4.value, "uri");
        value.uri = StringView_decode(v8);
    }
    {
        OptionalJSONValue v8 = json_get(&v4.value, "languageId");
        value.languageId = StringView_decode(v8);
    }
    {
        OptionalJSONValue v8 = json_get(&v4.value, "version");
        value.version = Int_decode(v8);
    }
    {
        OptionalJSONValue v8 = json_get(&v4.value, "text");
        value.text = StringView_decode(v8);
    }
    return value;
}

OptionalJSONValue TextDocumentItem_encode(TextDocumentItem value)
{
    JSONValue v4 = json_object();
    json_optional_set(&v4, "uri", StringView_encode(value.uri));
    json_optional_set(&v4, "languageId", StringView_encode(value.languageId));
    json_optional_set(&v4, "version", Int_encode(value.version));
    json_optional_set(&v4, "text", StringView_encode(value.text));
    RETURN_VALUE(JSONValue, v4);
}

OPTIONAL_JSON_ENCODE_IMPL(TextDocumentIdentifier);
OPTIONAL_JSON_DECODE_IMPL(TextDocumentIdentifier);

DA_IMPL(TextDocumentIdentifier);
DA_JSON_ENCODE_IMPL(TextDocumentIdentifier, TextDocumentIdentifiers, elements);
DA_JSON_DECODE_IMPL(TextDocumentIdentifier, TextDocumentIdentifiers, elements);
OPTIONAL_JSON_ENCODE_IMPL(TextDocumentIdentifiers);
OPTIONAL_JSON_DECODE_IMPL(TextDocumentIdentifiers);

TextDocumentIdentifier TextDocumentIdentifier_decode(OptionalJSONValue v4)
{
    assert(v4.has_value);
    assert(v4.value.type == JSON_TYPE_OBJECT);
    TextDocumentIdentifier value = {0};
    {
        OptionalJSONValue v8 = json_get(&v4.value, "uri");
        value.uri = StringView_decode(v8);
    }
    return value;
}

OptionalJSONValue TextDocumentIdentifier_encode(TextDocumentIdentifier value)
{
    JSONValue v4 = json_object();
    json_optional_set(&v4, "uri", StringView_encode(value.uri));
    RETURN_VALUE(JSONValue, v4);
}

OPTIONAL_JSON_ENCODE_IMPL(TextEdit);
OPTIONAL_JSON_DECODE_IMPL(TextEdit);

DA_IMPL(TextEdit);
DA_JSON_ENCODE_IMPL(TextEdit, TextEdits, elements);
DA_JSON_DECODE_IMPL(TextEdit, TextEdits, elements);
OPTIONAL_JSON_ENCODE_IMPL(TextEdits);
OPTIONAL_JSON_DECODE_IMPL(TextEdits);

TextEdit TextEdit_decode(OptionalJSONValue v4)
{
    assert(v4.has_value);
    assert(v4.value.type == JSON_TYPE_OBJECT);
    TextEdit value = {0};
    {
        OptionalJSONValue v8 = json_get(&v4.value, "range");
        value.range = Range_decode(v8);
    }
    {
        OptionalJSONValue v8 = json_get(&v4.value, "newText");
        value.newText = StringView_decode(v8);
    }
    return value;
}

OptionalJSONValue TextEdit_encode(TextEdit value)
{
    JSONValue v4 = json_object();
    json_optional_set(&v4, "range", Range_encode(value.range));
    json_optional_set(&v4, "newText", StringView_encode(value.newText));
    RETURN_VALUE(JSONValue, v4);
}

OPTIONAL_JSON_ENCODE_IMPL(VersionedTextDocumentIdentifier);
OPTIONAL_JSON_DECODE_IMPL(VersionedTextDocumentIdentifier);

DA_IMPL(VersionedTextDocumentIdentifier);
DA_JSON_ENCODE_IMPL(VersionedTextDocumentIdentifier, VersionedTextDocumentIdentifiers, elements);
DA_JSON_DECODE_IMPL(VersionedTextDocumentIdentifier, VersionedTextDocumentIdentifiers, elements);
OPTIONAL_JSON_ENCODE_IMPL(VersionedTextDocumentIdentifiers);
OPTIONAL_JSON_DECODE_IMPL(VersionedTextDocumentIdentifiers);

VersionedTextDocumentIdentifier VersionedTextDocumentIdentifier_decode(OptionalJSONValue v4)
{
    assert(v4.has_value);
    assert(v4.value.type == JSON_TYPE_OBJECT);
    VersionedTextDocumentIdentifier value = {0};
    {
        OptionalJSONValue v8 = json_get(&v4.value, "uri");
        value.uri = StringView_decode(v8);
    }
    {
        OptionalJSONValue v8 = json_get(&v4.value, "version");
        value.version = Int_decode(v8);
    }
    return value;
}

OptionalJSONValue VersionedTextDocumentIdentifier_encode(VersionedTextDocumentIdentifier value)
{
    JSONValue v4 = json_object();
    json_optional_set(&v4, "uri", StringView_encode(value.uri));
    json_optional_set(&v4, "version", Int_encode(value.version));
    RETURN_VALUE(JSONValue, v4);
}

// clang-format on

