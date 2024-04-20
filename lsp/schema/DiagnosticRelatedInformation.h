/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_DIAGNOSTICRELATEDINFORMATION_H__
#define __LSP_DIAGNOSTICRELATEDINFORMATION_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/Location.h>

typedef struct {
    Location   location;
    StringView message;
} DiagnosticRelatedInformation;

OPTIONAL(DiagnosticRelatedInformation);
DA_WITH_NAME(DiagnosticRelatedInformation, DiagnosticRelatedInformations);
OPTIONAL(DiagnosticRelatedInformations);

extern OptionalJSONValue                     DiagnosticRelatedInformation_encode(DiagnosticRelatedInformation value);
extern OptionalDiagnosticRelatedInformation  DiagnosticRelatedInformation_decode(OptionalJSONValue json);
extern OptionalJSONValue                     DiagnosticRelatedInformations_encode(DiagnosticRelatedInformations value);
extern OptionalDiagnosticRelatedInformations DiagnosticRelatedInformations_decode(OptionalJSONValue json);

#endif /* __LSP_DIAGNOSTICRELATEDINFORMATION_H__ */
