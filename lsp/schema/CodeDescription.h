/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_CODEDESCRIPTION_H__
#define __LSP_CODEDESCRIPTION_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/URI.h>

typedef struct {
    URI href;
} CodeDescription;

OPTIONAL(CodeDescription);
DA_WITH_NAME(CodeDescription, CodeDescriptions);
OPTIONAL(CodeDescriptions);

extern OptionalJSONValue        CodeDescription_encode(CodeDescription value);
extern OptionalCodeDescription  CodeDescription_decode(OptionalJSONValue json);
extern OptionalJSONValue        CodeDescriptions_encode(CodeDescriptions value);
extern OptionalCodeDescriptions CodeDescriptions_decode(OptionalJSONValue json);

#endif /* __LSP_CODEDESCRIPTION_H__ */
