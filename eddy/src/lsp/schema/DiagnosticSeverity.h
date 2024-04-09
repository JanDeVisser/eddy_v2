/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_DIAGNOSTICSEVERITY_H__
#define __LSP_DIAGNOSTICSEVERITY_H__

#include <lsp/schema/lsp_base.h>

typedef enum {
    DiagnosticSeverityError,
    DiagnosticSeverityWarning,
    DiagnosticSeverityInformation,
    DiagnosticSeverityHint,
} DiagnosticSeverity;

OPTIONAL(DiagnosticSeverity);
DA_WITH_NAME(DiagnosticSeverity, DiagnosticSeveritys);
OPTIONAL(DiagnosticSeveritys);

extern OptionalJSONValue           DiagnosticSeverity_encode(DiagnosticSeverity value);
extern OptionalDiagnosticSeverity  DiagnosticSeverity_decode(OptionalJSONValue json);
extern OptionalJSONValue           DiagnosticSeveritys_encode(DiagnosticSeveritys value);
extern OptionalDiagnosticSeveritys DiagnosticSeveritys_decode(OptionalJSONValue json);
#endif /* __LSP_DIAGNOSTICSEVERITY_H__ */
