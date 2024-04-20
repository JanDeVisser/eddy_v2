/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_WORKDONEPROGRESSPARAMS_H__
#define __LSP_WORKDONEPROGRESSPARAMS_H__

#include <lsp/schema/lsp_base.h>

typedef struct {
} WorkDoneProgressParams;

OPTIONAL(WorkDoneProgressParams);
DA_WITH_NAME(WorkDoneProgressParams, WorkDoneProgressParamss);
OPTIONAL(WorkDoneProgressParamss);

extern OptionalJSONValue               WorkDoneProgressParams_encode(WorkDoneProgressParams value);
extern OptionalWorkDoneProgressParams  WorkDoneProgressParams_decode(OptionalJSONValue json);
extern OptionalJSONValue               WorkDoneProgressParamss_encode(WorkDoneProgressParamss value);
extern OptionalWorkDoneProgressParamss WorkDoneProgressParamss_decode(OptionalJSONValue json);

#endif /* __LSP_WORKDONEPROGRESSPARAMS_H__ */
