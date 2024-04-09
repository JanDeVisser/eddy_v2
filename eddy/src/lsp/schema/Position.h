/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_POSITION_H__
#define __LSP_POSITION_H__

#include <lsp/schema/lsp_base.h>

typedef struct {
    unsigned int line;
    unsigned int character;
} Position;

OPTIONAL(Position);
DA_WITH_NAME(Position, Positions);
OPTIONAL(Positions);

extern OptionalJSONValue Position_encode(Position value);
extern OptionalPosition  Position_decode(OptionalJSONValue json);
extern OptionalJSONValue Positions_encode(Positions value);
extern OptionalPositions Positions_decode(OptionalJSONValue json);

#endif /* __LSP_POSITION_H__ */
