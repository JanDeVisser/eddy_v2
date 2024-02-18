/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */
 
// clang-format off 

#ifndef __LSP_SYNCHRONIZATION_H__
#define __LSP_SYNCHRONIZATION_H__

#include <lsp/lsp_base.h>
#include <lsp/base.h>

typedef struct {
    TextDocumentItem textDocument;
} DidOpenTextDocumentParams;

extern OptionalJSONValue DidOpenTextDocumentParams_encode(DidOpenTextDocumentParams value);

typedef struct {
    TextDocumentIdentifier textDocument;
    OptionalStringView text;
} DidSaveTextDocumentParams;

extern OptionalJSONValue DidSaveTextDocumentParams_encode(DidSaveTextDocumentParams value);

typedef struct {
    TextDocumentIdentifier textDocument;
} DidCloseTextDocumentParams;

extern OptionalJSONValue DidCloseTextDocumentParams_encode(DidCloseTextDocumentParams value);

typedef struct {
    Range range;
    OptionalUInt rangeLength;
    StringView text;
} TextDocumentContentChangeEvent;

DA_WITH_NAME(TextDocumentContentChangeEvent, TextDocumentContentChangeEvents);
JSON_ENCODE(TextDocumentContentChangeEvents, TextDocumentContentChangeEvents);
JSON_DECODE(TextDocumentContentChangeEvents, TextDocumentContentChangeEvents);

extern TextDocumentContentChangeEvent TextDocumentContentChangeEvent_decode(OptionalJSONValue value);
extern OptionalJSONValue TextDocumentContentChangeEvent_encode(TextDocumentContentChangeEvent value);

typedef struct {
    VersionedTextDocumentIdentifier textDocument;
    TextDocumentContentChangeEvents contentChanges;
} DidChangeTextDocumentParams;

extern OptionalJSONValue DidChangeTextDocumentParams_encode(DidChangeTextDocumentParams value);


// clang-format on
#endif /* __LSP_SYNCHRONIZATION_H__ */

