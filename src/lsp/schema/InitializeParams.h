/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#ifndef __LSP_INITIALIZEPARAMS_H__
#define __LSP_INITIALIZEPARAMS_H__

#include <lsp/schema/lsp_base.h>

#include <lsp/schema/ClientCapabilities.h>
#include <lsp/schema/DocumentUri.h>
#include <lsp/schema/TraceValue.h>
#include <lsp/schema/WorkspaceFolder.h>

typedef struct {
    struct {
        bool has_value;
        int  tag;
        union {
            int  _0;
            Null _1;
        };
    } processId;
    struct {
        bool               has_value;
        StringView         name;
        OptionalStringView version;
    } clientInfo;
    OptionalStringView locale;
    struct {
        bool has_value;
        int  tag;
        union {
            StringView _0;
            Null       _1;
        };
    } rootPath;
    struct {
        bool has_value;
        int  tag;
        union {
            DocumentUri _0;
            Null        _1;
        };
    } rootUri;
    OptionalJSONValue  initializationOptions;
    ClientCapabilities capabilities;
    OptionalTraceValue trace;
    struct {
        bool has_value;
        int  tag;
        union {
            WorkspaceFolders _0;
            Null             _1;
        };
    } workspaceFolders;
} InitializeParams;

OPTIONAL(InitializeParams);
OPTIONAL(OptionalInitializeParams);
DA_WITH_NAME(InitializeParams, InitializeParamss);
OPTIONAL(InitializeParamss);
OPTIONAL(OptionalInitializeParamss);

extern OptionalJSONValue                InitializeParams_encode(InitializeParams value);
extern OptionalInitializeParams         InitializeParams_decode(OptionalJSONValue json);
extern OptionalJSONValue                OptionalInitializeParams_encode(OptionalInitializeParams value);
extern OptionalOptionalInitializeParams OptionalInitializeParams_decode(OptionalJSONValue json);
extern OptionalJSONValue                InitializeParamss_encode(InitializeParamss value);
extern OptionalInitializeParamss        InitializeParamss_decode(OptionalJSONValue json);

#endif /* __LSP_INITIALIZEPARAMS_H__ */
