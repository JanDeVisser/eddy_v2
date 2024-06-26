#
# Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
#
# SPDX-License-Identifier: MIT
#
# THIS IS GENERATED CODE. DO NOT MODIFY.
#

include_directories(.)

add_library(
        lspschema
        lsp_base.c
        DocumentUri.c
URI.c
WorkDoneProgressParams.c
RegularExpressionsClientCapabilities.c
Position.c
PositionEncodingKind.c
MarkupKind.c
Range.c
TextDocumentItem.c
TextDocumentIdentifier.c
VersionedTextDocumentIdentifier.c
OptionalVersionedTextDocumentIdentifier.c
TextDocumentPositionParams.c
DocumentFilter.c
TextEdit.c
ChangeAnnotation.c
ChangeAnnotationIdentifier.c
AnnotatedTextEdit.c
Location.c
LocationLink.c
LSPCommand.c
MarkupContent.c
TraceValue.c
TokenFormat.c
SemanticTokenTypes.c
SemanticTokenModifiers.c
TextDocumentSyncKind.c
TextDocumentSyncClientCapabilities.c
SemanticTokensClientCapabilities.c
TextDocumentSyncClientCapabilities.c
TextDocumentClientCapabilities.c
TextDocumentClientCapabilities.c
ClientCapabilities.c
WorkspaceFolder.c
InitializeParams.c
SemanticTokensLegend.c
SemanticTokensOptions.c
SaveOptions.c
TextDocumentSyncOptions.c
ServerCapabilities.c
InitializeResult.c
SemanticTokensParams.c
SemanticTokens.c
DidOpenTextDocumentParams.c
DidSaveTextDocumentParams.c
DidCloseTextDocumentParams.c
TextDocumentContentChangeEvent.c
DidChangeTextDocumentParams.c
FormattingOptions.c
DocumentFormattingParams.c
DocumentRangeFormattingParams.c
CompletionTriggerKind.c
CompletionContext.c
CompletionParams.c
InsertTextFormat.c
CompletionItemTag.c
InsertReplaceEdit.c
InsertTextMode.c
CompletionItemLabelDetails.c
CompletionItemKind.c
CompletionItem.c
CompletionList.c
PublishDiagnosticsClientCapabilities.c
Diagnostic.c
DiagnosticSeverity.c
DiagnosticTag.c
DiagnosticRelatedInformation.c
CodeDescription.c
PublishDiagnosticsParams.c
)

target_link_libraries(lspschema base)

install(TARGETS lspschema
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
