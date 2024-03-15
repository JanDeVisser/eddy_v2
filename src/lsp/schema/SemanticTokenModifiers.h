/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_SEMANTICTOKENMODIFIERS_H__
#define __LSP_SEMANTICTOKENMODIFIERS_H__

#include <lsp/schema/lsp_base.h>

typedef enum {
    SemanticTokenModifiersDeclaration,
    SemanticTokenModifiersDefinition,
    SemanticTokenModifiersReadonly,
    SemanticTokenModifiersStatic,
    SemanticTokenModifiersDeprecated,
    SemanticTokenModifiersAbstract,
    SemanticTokenModifiersAsync,
    SemanticTokenModifiersModification,
    SemanticTokenModifiersDocumentation,
    SemanticTokenModifiersDefaultLibrary,
} SemanticTokenModifiers;

OPTIONAL(SemanticTokenModifiers);
DA_WITH_NAME(SemanticTokenModifiers, SemanticTokenModifierss);
OPTIONAL(SemanticTokenModifierss);

extern OptionalJSONValue               SemanticTokenModifiers_encode(SemanticTokenModifiers value);
extern OptionalSemanticTokenModifiers  SemanticTokenModifiers_decode(OptionalJSONValue json);
extern OptionalJSONValue               SemanticTokenModifierss_encode(SemanticTokenModifierss value);
extern OptionalSemanticTokenModifierss SemanticTokenModifierss_decode(OptionalJSONValue json);
extern StringView                      SemanticTokenModifiers_to_string(SemanticTokenModifiers value);
extern OptionalSemanticTokenModifiers  SemanticTokenModifiers_parse(StringView s);
#endif /* __LSP_SEMANTICTOKENMODIFIERS_H__ */
