/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_TRACEVALUE_H__
#define __LSP_TRACEVALUE_H__

#include <lsp/schema/lsp_base.h>

typedef enum {
    TraceValueOff,
    TraceValueMessages,
    TraceValueVerbose,
} TraceValue;

OPTIONAL(TraceValue);
OPTIONAL(OptionalTraceValue);
DA_WITH_NAME(TraceValue, TraceValues);
OPTIONAL(TraceValues);
OPTIONAL(OptionalTraceValues);

extern OptionalJSONValue           TraceValue_encode(TraceValue value);
extern OptionalTraceValue          TraceValue_decode(OptionalJSONValue json);
extern OptionalJSONValue           OptionalTraceValue_encode(OptionalTraceValue value);
extern OptionalOptionalTraceValue  OptionalTraceValue_decode(OptionalJSONValue json);
extern OptionalJSONValue           TraceValues_encode(TraceValues value);
extern OptionalTraceValues         TraceValues_decode(OptionalJSONValue json);
extern OptionalJSONValue           OptionalTraceValues_encode(OptionalTraceValues value);
extern OptionalOptionalTraceValues OptionalTraceValues_decode(OptionalJSONValue json);
extern StringView                  TraceValue_to_string(TraceValue value);
extern OptionalTraceValue          TraceValue_parse(StringView s);
#endif /* __LSP_TRACEVALUE_H__ */
