/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_ANNOTATEDTEXTEDIT_H__
#define __LSP_ANNOTATEDTEXTEDIT_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/ChangeAnnotationIdentifier.h>
#include <lsp/schema/TextEdit.h>

typedef struct {
    Range                      range;
    StringView                 newText;
    ChangeAnnotationIdentifier annotationId;
} AnnotatedTextEdit;

OPTIONAL(AnnotatedTextEdit);
DA_WITH_NAME(AnnotatedTextEdit, AnnotatedTextEdits);
OPTIONAL(AnnotatedTextEdits);

extern OptionalJSONValue          AnnotatedTextEdit_encode(AnnotatedTextEdit value);
extern OptionalAnnotatedTextEdit  AnnotatedTextEdit_decode(OptionalJSONValue json);
extern OptionalJSONValue          AnnotatedTextEdits_encode(AnnotatedTextEdits value);
extern OptionalAnnotatedTextEdits AnnotatedTextEdits_decode(OptionalJSONValue json);

#endif /* __LSP_ANNOTATEDTEXTEDIT_H__ */
