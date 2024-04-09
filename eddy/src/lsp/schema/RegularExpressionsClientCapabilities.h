/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_REGULAREXPRESSIONSCLIENTCAPABILITIES_H__
#define __LSP_REGULAREXPRESSIONSCLIENTCAPABILITIES_H__

#include <lsp/schema/lsp_base.h>

typedef struct {
    StringView         engine;
    OptionalStringView version;
} RegularExpressionsClientCapabilities;

OPTIONAL(RegularExpressionsClientCapabilities);
DA_WITH_NAME(RegularExpressionsClientCapabilities, RegularExpressionsClientCapabilitiess);
OPTIONAL(RegularExpressionsClientCapabilitiess);

extern OptionalJSONValue                             RegularExpressionsClientCapabilities_encode(RegularExpressionsClientCapabilities value);
extern OptionalRegularExpressionsClientCapabilities  RegularExpressionsClientCapabilities_decode(OptionalJSONValue json);
extern OptionalJSONValue                             RegularExpressionsClientCapabilitiess_encode(RegularExpressionsClientCapabilitiess value);
extern OptionalRegularExpressionsClientCapabilitiess RegularExpressionsClientCapabilitiess_decode(OptionalJSONValue json);

#endif /* __LSP_REGULAREXPRESSIONSCLIENTCAPABILITIES_H__ */
