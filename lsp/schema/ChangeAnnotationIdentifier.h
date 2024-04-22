/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_CHANGEANNOTATIONIDENTIFIER_H__
#define __LSP_CHANGEANNOTATIONIDENTIFIER_H__

#include <lsp/schema/lsp_base.h>

typedef StringView                    ChangeAnnotationIdentifier;
typedef OptionalStringView            OptionalChangeAnnotationIdentifier;
typedef DA_StringView                 DA_ChangeAnnotationIdentifier;
typedef DA_ChangeAnnotationIdentifier ChangeAnnotationIdentifiers;
typedef OptionalStringViews           OptionalChangeAnnotationIdentifiers;

#define ChangeAnnotationIdentifier_encode(V) StringView_encode(V)
#define ChangeAnnotationIdentifier_decode(V) StringView_decode(V)
#define ChangeAnnotationIdentifiers_encode(V) StringViews_encode(V)
#define ChangeAnnotationIdentifiers_decode(V) StringViews_decode(V)

#endif /* __LSP_CHANGEANNOTATIONIDENTIFIER_H__ */
