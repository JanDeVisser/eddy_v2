/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */
 
// clang-format off 

#ifndef __LSP_BASE_H__
#define __LSP_BASE_H__

#include <lsp/lsp_base.h>

typedef struct {
    unsigned int line;
    unsigned int character;
} Position;

OPTIONAL(Position);
OPTIONAL_JSON_ENCODE(Position);
OPTIONAL_JSON_DECODE(Position);

DA_WITH_NAME(Position, Positions);
JSON_ENCODE(Positions, Positions);
JSON_DECODE(Positions, Positions);
OPTIONAL(Positions);
OPTIONAL_JSON_ENCODE(Positions);
OPTIONAL_JSON_DECODE(Positions);

extern Position Position_decode(OptionalJSONValue value);
extern OptionalJSONValue Position_encode(Position value);

typedef struct {
    Position start;
    Position end;
} Range;

OPTIONAL(Range);
OPTIONAL_JSON_ENCODE(Range);
OPTIONAL_JSON_DECODE(Range);

DA_WITH_NAME(Range, Ranges);
JSON_ENCODE(Ranges, Ranges);
JSON_DECODE(Ranges, Ranges);
OPTIONAL(Ranges);
OPTIONAL_JSON_ENCODE(Ranges);
OPTIONAL_JSON_DECODE(Ranges);

extern Range Range_decode(OptionalJSONValue value);
extern OptionalJSONValue Range_encode(Range value);

typedef struct {
    StringView uri;
    StringView languageId;
    int version;
    StringView text;
} TextDocumentItem;

OPTIONAL(TextDocumentItem);
OPTIONAL_JSON_ENCODE(TextDocumentItem);
OPTIONAL_JSON_DECODE(TextDocumentItem);

DA_WITH_NAME(TextDocumentItem, TextDocumentItems);
JSON_ENCODE(TextDocumentItems, TextDocumentItems);
JSON_DECODE(TextDocumentItems, TextDocumentItems);
OPTIONAL(TextDocumentItems);
OPTIONAL_JSON_ENCODE(TextDocumentItems);
OPTIONAL_JSON_DECODE(TextDocumentItems);

extern TextDocumentItem TextDocumentItem_decode(OptionalJSONValue value);
extern OptionalJSONValue TextDocumentItem_encode(TextDocumentItem value);

typedef struct {
    StringView uri;
} TextDocumentIdentifier;

OPTIONAL(TextDocumentIdentifier);
OPTIONAL_JSON_ENCODE(TextDocumentIdentifier);
OPTIONAL_JSON_DECODE(TextDocumentIdentifier);

DA_WITH_NAME(TextDocumentIdentifier, TextDocumentIdentifiers);
JSON_ENCODE(TextDocumentIdentifiers, TextDocumentIdentifiers);
JSON_DECODE(TextDocumentIdentifiers, TextDocumentIdentifiers);
OPTIONAL(TextDocumentIdentifiers);
OPTIONAL_JSON_ENCODE(TextDocumentIdentifiers);
OPTIONAL_JSON_DECODE(TextDocumentIdentifiers);

extern TextDocumentIdentifier TextDocumentIdentifier_decode(OptionalJSONValue value);
extern OptionalJSONValue TextDocumentIdentifier_encode(TextDocumentIdentifier value);

typedef struct {
    Range range;
    StringView newText;
} TextEdit;

OPTIONAL(TextEdit);
OPTIONAL_JSON_ENCODE(TextEdit);
OPTIONAL_JSON_DECODE(TextEdit);

DA_WITH_NAME(TextEdit, TextEdits);
JSON_ENCODE(TextEdits, TextEdits);
JSON_DECODE(TextEdits, TextEdits);
OPTIONAL(TextEdits);
OPTIONAL_JSON_ENCODE(TextEdits);
OPTIONAL_JSON_DECODE(TextEdits);

extern TextEdit TextEdit_decode(OptionalJSONValue value);
extern OptionalJSONValue TextEdit_encode(TextEdit value);

typedef struct {
    StringView uri;
    int version;
} VersionedTextDocumentIdentifier;

OPTIONAL(VersionedTextDocumentIdentifier);
OPTIONAL_JSON_ENCODE(VersionedTextDocumentIdentifier);
OPTIONAL_JSON_DECODE(VersionedTextDocumentIdentifier);

DA_WITH_NAME(VersionedTextDocumentIdentifier, VersionedTextDocumentIdentifiers);
JSON_ENCODE(VersionedTextDocumentIdentifiers, VersionedTextDocumentIdentifiers);
JSON_DECODE(VersionedTextDocumentIdentifiers, VersionedTextDocumentIdentifiers);
OPTIONAL(VersionedTextDocumentIdentifiers);
OPTIONAL_JSON_ENCODE(VersionedTextDocumentIdentifiers);
OPTIONAL_JSON_DECODE(VersionedTextDocumentIdentifiers);

extern VersionedTextDocumentIdentifier VersionedTextDocumentIdentifier_decode(OptionalJSONValue value);
extern OptionalJSONValue VersionedTextDocumentIdentifier_encode(VersionedTextDocumentIdentifier value);


// clang-format on
#endif /* __LSP_BASE_H__ */

