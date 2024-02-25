type integer = number;
type uinteger = number;
type decimal = number;
type DocumentUri = string;
type URI = string;

export interface RegularExpressionsClientCapabilities {
    engine: string;
    version?: string;
}

interface Position {
    line: uinteger;
    character: uinteger;
}

export type PositionEncodingKind = string;
export namespace PositionEncodingKind {
    export const UTF8: PositionEncodingKind = 'utf-8';
    export const UTF16: PositionEncodingKind = 'utf-16';
    export const UTF32: PositionEncodingKind = 'utf-32';
}

interface Range {
    start: Position;
    end: Position;
}

interface TextDocumentItem {
    uri: DocumentUri;
    languageId: string;
    version: integer;
    text: string;
}

interface TextDocumentIdentifier {
    uri: DocumentUri;
}

interface VersionedTextDocumentIdentifier extends TextDocumentIdentifier {
    version: integer;
}

interface OptionalVersionedTextDocumentIdentifier extends TextDocumentIdentifier {
    version: integer | null;
}

interface TextDocumentPositionParams {
    textDocument: TextDocumentIdentifier;
    position: Position;
}

export interface DocumentFilter {
    language?: string;
    scheme?: string;
    pattern?: string;
}

interface TextEdit {
    range: Range;
    newText: string;
}

export interface ChangeAnnotation {
    label: string;
    needsConfirmation?: boolean;
    description?: string;
}

export type ChangeAnnotationIdentifier = string;

export interface AnnotatedTextEdit extends TextEdit {
    annotationId: ChangeAnnotationIdentifier;
}

// export interface TextDocumentEdit {
//     textDocument: OptionalVersionedTextDocumentIdentifier;
//     edits: (TextEdit | AnnotatedTextEdit)[];
// }

interface Location {
    uri: DocumentUri;
    range: Range;
}

interface LocationLink {
    originSelectionRange?: Range;
    targetUri: DocumentUri;
    targetRange: Range;
    targetSelectionRange: Range;
}
