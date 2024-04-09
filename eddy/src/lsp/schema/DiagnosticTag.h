/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_DIAGNOSTICTAG_H__
#define __LSP_DIAGNOSTICTAG_H__

#include <lsp/schema/lsp_base.h>

typedef enum {
    DiagnosticTagUnnecessary,
    DiagnosticTagDeprecated,
} DiagnosticTag;

OPTIONAL(DiagnosticTag);
DA_WITH_NAME(DiagnosticTag, DiagnosticTags);
OPTIONAL(DiagnosticTags);

extern OptionalJSONValue      DiagnosticTag_encode(DiagnosticTag value);
extern OptionalDiagnosticTag  DiagnosticTag_decode(OptionalJSONValue json);
extern OptionalJSONValue      DiagnosticTags_encode(DiagnosticTags value);
extern OptionalDiagnosticTags DiagnosticTags_decode(OptionalJSONValue json);
#endif /* __LSP_DIAGNOSTICTAG_H__ */
