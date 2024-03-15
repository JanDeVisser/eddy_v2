/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_SEMANTICTOKENTYPES_H__
#define __LSP_SEMANTICTOKENTYPES_H__

#include <lsp/schema/lsp_base.h>

typedef enum {
    SemanticTokenTypesNamespace,
    SemanticTokenTypesType,
    SemanticTokenTypesClass,
    SemanticTokenTypesEnum,
    SemanticTokenTypesInterface,
    SemanticTokenTypesStruct,
    SemanticTokenTypesTypeParameter,
    SemanticTokenTypesParameter,
    SemanticTokenTypesVariable,
    SemanticTokenTypesProperty,
    SemanticTokenTypesEnumMember,
    SemanticTokenTypesEvent,
    SemanticTokenTypesFunction,
    SemanticTokenTypesMethod,
    SemanticTokenTypesMacro,
    SemanticTokenTypesKeyword,
    SemanticTokenTypesModifier,
    SemanticTokenTypesComment,
    SemanticTokenTypesString,
    SemanticTokenTypesNumber,
    SemanticTokenTypesRegexp,
    SemanticTokenTypesOperator,
    SemanticTokenTypesDecorator,
} SemanticTokenTypes;

OPTIONAL(SemanticTokenTypes);
OPTIONAL(OptionalSemanticTokenTypes);
DA_WITH_NAME(SemanticTokenTypes, SemanticTokenTypess);
OPTIONAL(SemanticTokenTypess);
OPTIONAL(OptionalSemanticTokenTypess);

extern OptionalJSONValue                   SemanticTokenTypes_encode(SemanticTokenTypes value);
extern OptionalSemanticTokenTypes          SemanticTokenTypes_decode(OptionalJSONValue json);
extern OptionalJSONValue                   OptionalSemanticTokenTypes_encode(OptionalSemanticTokenTypes value);
extern OptionalOptionalSemanticTokenTypes  OptionalSemanticTokenTypes_decode(OptionalJSONValue json);
extern OptionalJSONValue                   SemanticTokenTypess_encode(SemanticTokenTypess value);
extern OptionalSemanticTokenTypess         SemanticTokenTypess_decode(OptionalJSONValue json);
extern OptionalJSONValue                   OptionalSemanticTokenTypess_encode(OptionalSemanticTokenTypess value);
extern OptionalOptionalSemanticTokenTypess OptionalSemanticTokenTypess_decode(OptionalJSONValue json);
extern StringView                          SemanticTokenTypes_to_string(SemanticTokenTypes value);
extern OptionalSemanticTokenTypes          SemanticTokenTypes_parse(StringView s);
#endif /* __LSP_SEMANTICTOKENTYPES_H__ */
