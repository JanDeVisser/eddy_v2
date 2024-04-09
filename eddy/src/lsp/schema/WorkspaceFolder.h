/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_WORKSPACEFOLDER_H__
#define __LSP_WORKSPACEFOLDER_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/URI.h>

typedef struct {
    URI        uri;
    StringView name;
} WorkspaceFolder;

OPTIONAL(WorkspaceFolder);
DA_WITH_NAME(WorkspaceFolder, WorkspaceFolders);
OPTIONAL(WorkspaceFolders);

extern OptionalJSONValue        WorkspaceFolder_encode(WorkspaceFolder value);
extern OptionalWorkspaceFolder  WorkspaceFolder_decode(OptionalJSONValue json);
extern OptionalJSONValue        WorkspaceFolders_encode(WorkspaceFolders value);
extern OptionalWorkspaceFolders WorkspaceFolders_decode(OptionalJSONValue json);

#endif /* __LSP_WORKSPACEFOLDER_H__ */
