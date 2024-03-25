/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_PUBLISHDIAGNOSTICSPARAMS_H__
#define __LSP_PUBLISHDIAGNOSTICSPARAMS_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/Diagnostic.h>
#include <lsp/schema/DocumentUri.h>

typedef struct {
    DocumentUri uri;
    OptionalInt version;
    Diagnostics diagnostics;
} PublishDiagnosticsParams;

OPTIONAL(PublishDiagnosticsParams);
DA_WITH_NAME(PublishDiagnosticsParams, PublishDiagnosticsParamss);
OPTIONAL(PublishDiagnosticsParamss);

extern OptionalJSONValue                 PublishDiagnosticsParams_encode(PublishDiagnosticsParams value);
extern OptionalPublishDiagnosticsParams  PublishDiagnosticsParams_decode(OptionalJSONValue json);
extern OptionalJSONValue                 PublishDiagnosticsParamss_encode(PublishDiagnosticsParamss value);
extern OptionalPublishDiagnosticsParamss PublishDiagnosticsParamss_decode(OptionalJSONValue json);

#endif /* __LSP_PUBLISHDIAGNOSTICSPARAMS_H__ */
