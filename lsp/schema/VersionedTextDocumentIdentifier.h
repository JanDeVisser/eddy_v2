/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_VERSIONEDTEXTDOCUMENTIDENTIFIER_H__
#define __LSP_VERSIONEDTEXTDOCUMENTIDENTIFIER_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/TextDocumentIdentifier.h>

typedef struct {
    DocumentUri uri;
    int         version;
} VersionedTextDocumentIdentifier;

OPTIONAL(VersionedTextDocumentIdentifier);
DA_WITH_NAME(VersionedTextDocumentIdentifier, VersionedTextDocumentIdentifiers);
OPTIONAL(VersionedTextDocumentIdentifiers);

extern OptionalJSONValue                        VersionedTextDocumentIdentifier_encode(VersionedTextDocumentIdentifier value);
extern OptionalVersionedTextDocumentIdentifier  VersionedTextDocumentIdentifier_decode(OptionalJSONValue json);
extern OptionalJSONValue                        VersionedTextDocumentIdentifiers_encode(VersionedTextDocumentIdentifiers value);
extern OptionalVersionedTextDocumentIdentifiers VersionedTextDocumentIdentifiers_decode(OptionalJSONValue json);

#endif /* __LSP_VERSIONEDTEXTDOCUMENTIDENTIFIER_H__ */
