/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_PUBLISHDIAGNOSTICSCLIENTCAPABILITIES_H__
#define __LSP_PUBLISHDIAGNOSTICSCLIENTCAPABILITIES_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/DiagnosticTag.h>

typedef struct {
    OptionalBool relatedInformation;
    struct {
        bool           has_value;
        DiagnosticTags valueSet;
    } tagSupport;
    OptionalBool versionSupport;
    OptionalBool codeDescriptionSupport;
    OptionalBool dataSupport;
} PublishDiagnosticsClientCapabilities;

OPTIONAL(PublishDiagnosticsClientCapabilities);
DA_WITH_NAME(PublishDiagnosticsClientCapabilities, PublishDiagnosticsClientCapabilitiess);
OPTIONAL(PublishDiagnosticsClientCapabilitiess);

extern OptionalJSONValue                             PublishDiagnosticsClientCapabilities_encode(PublishDiagnosticsClientCapabilities value);
extern OptionalPublishDiagnosticsClientCapabilities  PublishDiagnosticsClientCapabilities_decode(OptionalJSONValue json);
extern OptionalJSONValue                             PublishDiagnosticsClientCapabilitiess_encode(PublishDiagnosticsClientCapabilitiess value);
extern OptionalPublishDiagnosticsClientCapabilitiess PublishDiagnosticsClientCapabilitiess_decode(OptionalJSONValue json);

#endif /* __LSP_PUBLISHDIAGNOSTICSCLIENTCAPABILITIES_H__ */
