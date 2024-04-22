/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_SAVEOPTIONS_H__
#define __LSP_SAVEOPTIONS_H__

#include <lsp/schema/lsp_base.h>

typedef struct {
    OptionalBool includeText;
} SaveOptions;

OPTIONAL(SaveOptions);
DA_WITH_NAME(SaveOptions, SaveOptionss);
OPTIONAL(SaveOptionss);

extern OptionalJSONValue    SaveOptions_encode(SaveOptions value);
extern OptionalSaveOptions  SaveOptions_decode(OptionalJSONValue json);
extern OptionalJSONValue    SaveOptionss_encode(SaveOptionss value);
extern OptionalSaveOptionss SaveOptionss_decode(OptionalJSONValue json);

#endif /* __LSP_SAVEOPTIONS_H__ */
