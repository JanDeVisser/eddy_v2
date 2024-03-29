/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_URI_H__
#define __LSP_URI_H__

#include <lsp/schema/lsp_base.h>

typedef StringView          URI;
typedef OptionalStringView  OptionalURI;
typedef DA_StringView       DA_URI;
typedef DA_URI              URIs;
typedef OptionalStringViews OptionalURIs;

#define URI_encode(V) StringView_encode(V)
#define URI_decode(V) StringView_decode(V)
#define URIs_encode(V) StringViews_encode(V)
#define URIs_decode(V) StringViews_decode(V)

#endif /* __LSP_URI_H__ */
