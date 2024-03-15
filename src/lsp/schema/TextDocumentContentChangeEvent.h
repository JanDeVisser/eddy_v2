/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_TEXTDOCUMENTCONTENTCHANGEEVENT_H__
#define __LSP_TEXTDOCUMENTCONTENTCHANGEEVENT_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/Range.h>

typedef struct {
    bool has_value;
    int  tag;
    union {
        struct {
            bool           has_value;
            Range          range;
            OptionalUInt32 rangeLength;
            StringView     text;
        } _0;
        struct {
            bool       has_value;
            StringView text;
        } _1;
    };
} TextDocumentContentChangeEvent;

OPTIONAL(TextDocumentContentChangeEvent);
DA_WITH_NAME(TextDocumentContentChangeEvent, TextDocumentContentChangeEvents);
OPTIONAL(TextDocumentContentChangeEvents);

extern OptionalJSONValue                       TextDocumentContentChangeEvent_encode(TextDocumentContentChangeEvent value);
extern OptionalTextDocumentContentChangeEvent  TextDocumentContentChangeEvent_decode(OptionalJSONValue json);
extern OptionalJSONValue                       TextDocumentContentChangeEvents_encode(TextDocumentContentChangeEvents value);
extern OptionalTextDocumentContentChangeEvents TextDocumentContentChangeEvents_decode(OptionalJSONValue json);
#endif /* __LSP_TEXTDOCUMENTCONTENTCHANGEEVENT_H__ */
