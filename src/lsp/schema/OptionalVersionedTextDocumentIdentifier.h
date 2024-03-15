/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_OPTIONALVERSIONEDTEXTDOCUMENTIDENTIFIER_H__
#define __LSP_OPTIONALVERSIONEDTEXTDOCUMENTIDENTIFIER_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/TextDocumentIdentifier.h>

typedef struct {
    DocumentUri uri;
    struct {
        bool has_value;
        int  tag;
        union {
            int  _0;
            Null _1;
        };
    } version;
} OptionalVersionedTextDocumentIdentifier;

OPTIONAL(OptionalVersionedTextDocumentIdentifier);
DA_WITH_NAME(OptionalVersionedTextDocumentIdentifier, OptionalVersionedTextDocumentIdentifiers);
OPTIONAL(OptionalVersionedTextDocumentIdentifiers);

extern OptionalJSONValue                                OptionalVersionedTextDocumentIdentifier_encode(OptionalVersionedTextDocumentIdentifier value);
extern OptionalOptionalVersionedTextDocumentIdentifier  OptionalVersionedTextDocumentIdentifier_decode(OptionalJSONValue json);
extern OptionalJSONValue                                OptionalVersionedTextDocumentIdentifiers_encode(OptionalVersionedTextDocumentIdentifiers value);
extern OptionalOptionalVersionedTextDocumentIdentifiers OptionalVersionedTextDocumentIdentifiers_decode(OptionalJSONValue json);

#endif /* __LSP_OPTIONALVERSIONEDTEXTDOCUMENTIDENTIFIER_H__ */
