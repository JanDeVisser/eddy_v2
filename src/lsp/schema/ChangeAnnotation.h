/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_CHANGEANNOTATION_H__
#define __LSP_CHANGEANNOTATION_H__

#include <lsp/schema/lsp_base.h>

typedef struct {
    StringView         label;
    OptionalBool       needsConfirmation;
    OptionalStringView description;
} ChangeAnnotation;

OPTIONAL(ChangeAnnotation);
OPTIONAL(OptionalChangeAnnotation);
DA_WITH_NAME(ChangeAnnotation, ChangeAnnotations);
OPTIONAL(ChangeAnnotations);
OPTIONAL(OptionalChangeAnnotations);

extern OptionalJSONValue                ChangeAnnotation_encode(ChangeAnnotation value);
extern OptionalChangeAnnotation         ChangeAnnotation_decode(OptionalJSONValue json);
extern OptionalJSONValue                OptionalChangeAnnotation_encode(OptionalChangeAnnotation value);
extern OptionalOptionalChangeAnnotation OptionalChangeAnnotation_decode(OptionalJSONValue json);
extern OptionalJSONValue                ChangeAnnotations_encode(ChangeAnnotations value);
extern OptionalChangeAnnotations        ChangeAnnotations_decode(OptionalJSONValue json);

#endif /* __LSP_CHANGEANNOTATION_H__ */
