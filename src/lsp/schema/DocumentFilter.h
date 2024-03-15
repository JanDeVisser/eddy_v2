/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_DOCUMENTFILTER_H__
#define __LSP_DOCUMENTFILTER_H__

#include <lsp/schema/lsp_base.h>

typedef struct {
    OptionalStringView language;
    OptionalStringView scheme;
    OptionalStringView pattern;
} DocumentFilter;

OPTIONAL(DocumentFilter);
OPTIONAL(OptionalDocumentFilter);
DA_WITH_NAME(DocumentFilter, DocumentFilters);
OPTIONAL(DocumentFilters);
OPTIONAL(OptionalDocumentFilters);

extern OptionalJSONValue              DocumentFilter_encode(DocumentFilter value);
extern OptionalDocumentFilter         DocumentFilter_decode(OptionalJSONValue json);
extern OptionalJSONValue              OptionalDocumentFilter_encode(OptionalDocumentFilter value);
extern OptionalOptionalDocumentFilter OptionalDocumentFilter_decode(OptionalJSONValue json);
extern OptionalJSONValue              DocumentFilters_encode(DocumentFilters value);
extern OptionalDocumentFilters        DocumentFilters_decode(OptionalJSONValue json);

#endif /* __LSP_DOCUMENTFILTER_H__ */
