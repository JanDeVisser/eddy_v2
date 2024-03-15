/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_POSITIONENCODINGKIND_H__
#define __LSP_POSITIONENCODINGKIND_H__

#include <lsp/schema/lsp_base.h>

typedef enum {
    PositionEncodingKindUTF8,
    PositionEncodingKindUTF16,
    PositionEncodingKindUTF32,
} PositionEncodingKind;

OPTIONAL(PositionEncodingKind);
OPTIONAL(OptionalPositionEncodingKind);
DA_WITH_NAME(PositionEncodingKind, PositionEncodingKinds);
OPTIONAL(PositionEncodingKinds);
OPTIONAL(OptionalPositionEncodingKinds);

extern OptionalJSONValue                     PositionEncodingKind_encode(PositionEncodingKind value);
extern OptionalPositionEncodingKind          PositionEncodingKind_decode(OptionalJSONValue json);
extern OptionalJSONValue                     OptionalPositionEncodingKind_encode(OptionalPositionEncodingKind value);
extern OptionalOptionalPositionEncodingKind  OptionalPositionEncodingKind_decode(OptionalJSONValue json);
extern OptionalJSONValue                     PositionEncodingKinds_encode(PositionEncodingKinds value);
extern OptionalPositionEncodingKinds         PositionEncodingKinds_decode(OptionalJSONValue json);
extern OptionalJSONValue                     OptionalPositionEncodingKinds_encode(OptionalPositionEncodingKinds value);
extern OptionalOptionalPositionEncodingKinds OptionalPositionEncodingKinds_decode(OptionalJSONValue json);
extern StringView                            PositionEncodingKind_to_string(PositionEncodingKind value);
extern OptionalPositionEncodingKind          PositionEncodingKind_parse(StringView s);
#endif /* __LSP_POSITIONENCODINGKIND_H__ */
