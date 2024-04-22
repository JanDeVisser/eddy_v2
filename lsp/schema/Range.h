/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_RANGE_H__
#define __LSP_RANGE_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/Position.h>

typedef struct {
    Position start;
    Position end;
} Range;

OPTIONAL(Range);
DA_WITH_NAME(Range, Ranges);
OPTIONAL(Ranges);

extern OptionalJSONValue Range_encode(Range value);
extern OptionalRange     Range_decode(OptionalJSONValue json);
extern OptionalJSONValue Ranges_encode(Ranges value);
extern OptionalRanges    Ranges_decode(OptionalJSONValue json);

#endif /* __LSP_RANGE_H__ */
