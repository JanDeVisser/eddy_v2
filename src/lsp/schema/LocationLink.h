/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_LOCATIONLINK_H__
#define __LSP_LOCATIONLINK_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/DocumentUri.h>
#include <lsp/schema/Range.h>

typedef struct {
    OptionalRange originSelectionRange;
    DocumentUri   targetUri;
    Range         targetRange;
    Range         targetSelectionRange;
} LocationLink;

OPTIONAL(LocationLink);
OPTIONAL(OptionalLocationLink);
DA_WITH_NAME(LocationLink, LocationLinks);
OPTIONAL(LocationLinks);
OPTIONAL(OptionalLocationLinks);

extern OptionalJSONValue            LocationLink_encode(LocationLink value);
extern OptionalLocationLink         LocationLink_decode(OptionalJSONValue json);
extern OptionalJSONValue            OptionalLocationLink_encode(OptionalLocationLink value);
extern OptionalOptionalLocationLink OptionalLocationLink_decode(OptionalJSONValue json);
extern OptionalJSONValue            LocationLinks_encode(LocationLinks value);
extern OptionalLocationLinks        LocationLinks_decode(OptionalJSONValue json);

#endif /* __LSP_LOCATIONLINK_H__ */
