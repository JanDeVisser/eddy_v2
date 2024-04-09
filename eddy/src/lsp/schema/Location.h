/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_LOCATION_H__
#define __LSP_LOCATION_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/DocumentUri.h>
#include <lsp/schema/Range.h>

typedef struct {
    DocumentUri uri;
    Range       range;
} Location;

OPTIONAL(Location);
DA_WITH_NAME(Location, Locations);
OPTIONAL(Locations);

extern OptionalJSONValue Location_encode(Location value);
extern OptionalLocation  Location_decode(OptionalJSONValue json);
extern OptionalJSONValue Locations_encode(Locations value);
extern OptionalLocations Locations_decode(OptionalJSONValue json);

#endif /* __LSP_LOCATION_H__ */
