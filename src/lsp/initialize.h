/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */
 
// clang-format off 

#ifndef __LSP_INITIALIZE_H__
#define __LSP_INITIALIZE_H__

#include <lsp/lsp_base.h>

typedef enum {
    TraceValueOff,
    TraceValueMessages,
    TraceValueVerbose,
} TraceValue;

OPTIONAL(TraceValue);
OPTIONAL_JSON(TraceValue);

extern StringView TraceValue_to_string(TraceValue value);
extern TraceValue TraceValue_parse(StringView s);
extern TraceValue TraceValue_decode(OptionalJSONValue value);
extern OptionalJSONValue TraceValue_encode(TraceValue value);

typedef enum {
    TokenFormatRelative,
} TokenFormat;

DA_WITH_NAME(TokenFormat, TokenFormats);
JSON(TokenFormats, TokenFormats);

extern StringView TokenFormat_to_string(TokenFormat value);
extern TokenFormat TokenFormat_parse(StringView s);
extern TokenFormat TokenFormat_decode(OptionalJSONValue value);
extern OptionalJSONValue TokenFormat_encode(TokenFormat value);

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

DA_WITH_NAME(SemanticTokenTypes, SemanticTokenTypess);
JSON(SemanticTokenTypess, SemanticTokenTypess);

extern StringView SemanticTokenTypes_to_string(SemanticTokenTypes value);
extern SemanticTokenTypes SemanticTokenTypes_parse(StringView s);
extern SemanticTokenTypes SemanticTokenTypes_decode(OptionalJSONValue value);
extern OptionalJSONValue SemanticTokenTypes_encode(SemanticTokenTypes value);

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

extern StringView SemanticTokenModifiers_to_string(SemanticTokenModifiers value);
extern SemanticTokenModifiers SemanticTokenModifiers_parse(StringView s);
extern SemanticTokenModifiers SemanticTokenModifiers_decode(OptionalJSONValue value);
extern OptionalJSONValue SemanticTokenModifiers_encode(SemanticTokenModifiers value);

typedef struct {
    OptionalBool dynamicRegistration;
    struct {
        bool has_value;
        struct {
            struct {
                bool has_value;
                int tag;
                union {
                    bool _0;
                    Empty _1;
                };
            } range;
            struct {
                bool has_value;
                int tag;
                union {
                    bool _0;
                    struct {
                        bool has_value;
                        struct {
                            OptionalBool delta;
                        };
                    } _1;
                };
            } full;
        };
    } requests;
    StringList tokenTypes;
    StringList tokenModifiers;
    TokenFormats formats;
    OptionalBool overlappingTokenSupport;
    OptionalBool multilineTokenSupport;
    OptionalBool serverCancelSupport;
} SemanticTokensClientCapabilities;

OPTIONAL(SemanticTokensClientCapabilities);
OPTIONAL_JSON_ENCODE(SemanticTokensClientCapabilities);

extern OptionalJSONValue SemanticTokensClientCapabilities_encode(SemanticTokensClientCapabilities value);

typedef struct {
    OptionalSemanticTokensClientCapabilities semanticTokens;
} TextDocumentClientCapabilities;

OPTIONAL(TextDocumentClientCapabilities);
OPTIONAL_JSON_ENCODE(TextDocumentClientCapabilities);

extern OptionalJSONValue TextDocumentClientCapabilities_encode(TextDocumentClientCapabilities value);

typedef struct {
    OptionalTextDocumentClientCapabilities textDocument;
} ClientCapabilities;

extern OptionalJSONValue ClientCapabilities_encode(ClientCapabilities value);

typedef struct {
    StringView uri;
    StringView name;
} WorkspaceFolder;

DA_WITH_NAME(WorkspaceFolder, WorkspaceFolders);
JSON_ENCODE(WorkspaceFolders, WorkspaceFolders);
JSON_DECODE(WorkspaceFolders, WorkspaceFolders);

extern WorkspaceFolder WorkspaceFolder_decode(OptionalJSONValue value);
extern OptionalJSONValue WorkspaceFolder_encode(WorkspaceFolder value);

typedef struct {
    struct {
        bool has_value;
        int tag;
        union {
            int _0;
            Null _1;
        };
    } processId;
    struct {
        bool has_value;
        struct {
            StringView name;
            OptionalStringView version;
        };
    } clientInfo;
    OptionalStringView locale;
    OptionalStringView rootPath;
    OptionalStringView rootUri;
    OptionalJSONValue initializationOptions;
    ClientCapabilities capabilities;
    OptionalTraceValue trace;
    struct {
        bool has_value;
        int tag;
        union {
            WorkspaceFolders _0;
            Null _1;
        };
    } workspaceFolders;
} InitializeParams;

extern OptionalJSONValue InitializeParams_encode(InitializeParams value);

typedef struct {
    StringList tokenTypes;
    StringList tokenModifiers;
} SemanticTokensLegend;

extern SemanticTokensLegend SemanticTokensLegend_decode(OptionalJSONValue value);
extern OptionalJSONValue SemanticTokensLegend_encode(SemanticTokensLegend value);

typedef struct {
    SemanticTokensLegend legend;
    struct {
        bool has_value;
        int tag;
        union {
            bool _0;
            Empty _1;
        };
    } range;
    struct {
        bool has_value;
        int tag;
        union {
            bool _0;
            struct {
                bool has_value;
                struct {
                    OptionalBool delta;
                };
            } _1;
        };
    } full;
} SemanticTokensOptions;

OPTIONAL(SemanticTokensOptions);
OPTIONAL_JSON_DECODE(SemanticTokensOptions);

extern SemanticTokensOptions SemanticTokensOptions_decode(OptionalJSONValue value);

typedef struct {
    OptionalSemanticTokensOptions semanticTokensProvider;
} ServerCapabilities;

extern ServerCapabilities ServerCapabilities_decode(OptionalJSONValue value);

typedef struct {
    ServerCapabilities capabilities;
    struct {
        bool has_value;
        struct {
            StringView name;
            OptionalStringView version;
        };
    } serverInfo;
} InitializeResult;

extern InitializeResult InitializeResult_decode(OptionalJSONValue value);


// clang-format on
#endif /* __LSP_INITIALIZE_H__ */

