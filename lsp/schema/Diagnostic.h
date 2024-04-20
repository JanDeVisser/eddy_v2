/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_DIAGNOSTIC_H__
#define __LSP_DIAGNOSTIC_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/CodeDescription.h>
#include <lsp/schema/DiagnosticRelatedInformation.h>
#include <lsp/schema/DiagnosticSeverity.h>
#include <lsp/schema/DiagnosticTag.h>
#include <lsp/schema/Range.h>

typedef struct {
    Range                      range;
    OptionalDiagnosticSeverity severity;
    struct {
        bool has_value;
        int  tag;
        union {
            int        _0;
            StringView _1;
        };
    } code;
    OptionalCodeDescription               codeDescription;
    OptionalStringView                    source;
    StringView                            message;
    OptionalDiagnosticTags                tags;
    OptionalDiagnosticRelatedInformations relatedInformation;
    OptionalJSONValue                     data;
} Diagnostic;

OPTIONAL(Diagnostic);
DA_WITH_NAME(Diagnostic, Diagnostics);
OPTIONAL(Diagnostics);

extern OptionalJSONValue   Diagnostic_encode(Diagnostic value);
extern OptionalDiagnostic  Diagnostic_decode(OptionalJSONValue json);
extern OptionalJSONValue   Diagnostics_encode(Diagnostics value);
extern OptionalDiagnostics Diagnostics_decode(OptionalJSONValue json);

#endif /* __LSP_DIAGNOSTIC_H__ */
