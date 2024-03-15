/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_DOCUMENTURI_H__
#define __LSP_DOCUMENTURI_H__

#include <lsp/schema/lsp_base.h>

typedef StringView                  DocumentUri;
typedef OptionalStringView          OptionalDocumentUri;
typedef OptionalOptionalStringView  OptionalOptionalDocumentUri;
typedef DA_StringView               DA_DocumentUri;
typedef DA_DocumentUri              DocumentUris;
typedef OptionalStringViews         OptionalDocumentUris;
typedef OptionalOptionalStringViews OptionalOptionalDocumentUris;

#define DocumentUri_encode(V) StringView_encode(V)
#define DocumentUri_decode(V) StringView_decode(V)
#define OptionalDocumentUri_encode(V) OptionalStringView_encode(V)
#define OptionalDocumentUri_decode(V) OptionalStringView_decode(V)
#define DocumentUris_encode(V) StringViews_encode(V)
#define DocumentUris_decode(V) StringViews_decode(V)
#define OptionalDocumentUris_encode(V) OptionalStringViews_encode(V)
#define OptionalDocumentUris_decode(V) OptionalStringViews_decode(V)

#endif /* __LSP_DOCUMENTURI_H__ */
