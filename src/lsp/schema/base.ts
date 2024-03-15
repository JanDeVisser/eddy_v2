type integer = number;
type uinteger = number;
type decimal = number;
type DocumentUri = string;
type URI = string;

export interface WorkDoneProgressParams {
}

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

export enum TraceValue {
    off = 'off',
    messages = 'messages',
    verbose = 'verbose',
}

export enum TokenFormat {
	Relative = 'relative',
}

export enum SemanticTokenTypes {
	namespace = 'namespace',
	type = 'type',
	class = 'class',
	enum = 'enum',
	interface = 'interface',
	struct = 'struct',
	typeParameter = 'typeParameter',
	parameter = 'parameter',
	variable = 'variable',
	property = 'property',
	enumMember = 'enumMember',
	event = 'event',
	function = 'function',
	method = 'method',
	macro = 'macro',
	keyword = 'keyword',
	modifier = 'modifier',
	comment = 'comment',
	string = 'string',
	number = 'number',
	regexp = 'regexp',
	operator = 'operator',
	decorator = 'decorator',
}

export enum SemanticTokenModifiers {
	declaration = 'declaration',
	definition = 'definition',
	readonly = 'readonly',
	static = 'static',
	deprecated = 'deprecated',
	abstract = 'abstract',
	async = 'async',
	modification = 'modification',
	documentation = 'documentation',
	defaultLibrary = 'defaultLibrary'
}

export enum TextDocumentSyncKind {
	none = 0,
	full = 1,
	incremental = 2,
}

export interface TextDocumentSyncClientCapabilities {
	dynamicRegistration?: boolean;
	willSave?: boolean;
	willSaveWaitUntil?: boolean;
	didSave?: boolean;
}

export interface SemanticTokensClientCapabilities {
	dynamicRegistration?: boolean;
	requests: {
		range?: boolean | {
		};
		full?: boolean | {
			delta?: boolean;
		};
	};
	tokenTypes: string[];
	tokenModifiers: string[];
	formats: TokenFormat[];
	overlappingTokenSupport?: boolean;
	multilineTokenSupport?: boolean;
	serverCancelSupport?: boolean;
	augmentsSyntaxTokens?: boolean;
}

export interface TextDocumentSyncClientCapabilities {
	dynamicRegistration?: boolean;
	willSave?: boolean;
	willSaveWaitUntil?: boolean;
	didSave?: boolean;
}

export interface TextDocumentClientCapabilities {
	synchronization?: TextDocumentSyncClientCapabilities;
	semanticTokens?: SemanticTokensClientCapabilities;
}

export interface TextDocumentClientCapabilities {
	synchronization?: TextDocumentSyncClientCapabilities;
	semanticTokens?: SemanticTokensClientCapabilities;
}

export interface ClientCapabilities {
	textDocument?: TextDocumentClientCapabilities;
}

export interface WorkspaceFolder {
	uri: URI;
	name: string;
}

export interface InitializeParams {
	processId: integer | null;
	clientInfo?: {
		name: string;
		version?: string;
	};
	locale?: string;
	rootPath?: string | null;
	rootUri: DocumentUri | null;
	initializationOptions?: LSPAny;
	capabilities: ClientCapabilities;
	trace?: TraceValue;
	workspaceFolders?: WorkspaceFolder[] | null;
}

export interface SemanticTokensLegend {
	tokenTypes: string[];
	tokenModifiers: string[];
}

export interface SemanticTokensOptions {
	legend: SemanticTokensLegend;
	range?: boolean | {
	};
	full?: boolean | {
		delta?: boolean;
	};
}

export interface SaveOptions {
	includeText?: boolean;
}

export interface TextDocumentSyncOptions {
	openClose?: boolean;
	change?: TextDocumentSyncKind;
    willSave?: boolean;
	willSaveWaitUntil?: boolean;
	save?: boolean | SaveOptions;
}

interface ServerCapabilities {
	positionEncoding?: PositionEncodingKind;
	textDocumentSync?: TextDocumentSyncOptions | TextDocumentSyncKind;
	semanticTokensProvider?: SemanticTokensOptions;
}

interface InitializeResult {
	capabilities: ServerCapabilities;
	serverInfo?: {
		name: string;
		version?: string;
	};
}

export interface SemanticTokensParams  {
	textDocument: TextDocumentIdentifier;
}

export interface SemanticTokens {
	resultId?: string;
	data: uinteger[];
}

interface DidOpenTextDocumentParams {
	textDocument: TextDocumentItem;
}

interface DidSaveTextDocumentParams {
	textDocument: TextDocumentIdentifier;
	text?: string;
}

interface DidCloseTextDocumentParams {
	textDocument: TextDocumentIdentifier;
}

export type TextDocumentContentChangeEvent = {
	range: Range;
	rangeLength?: uinteger;
	text: string;
} | {
	text: string;
};

interface DidChangeTextDocumentParams {
	textDocument: VersionedTextDocumentIdentifier;
	contentChanges: TextDocumentContentChangeEvent[];
}

interface FormattingOptions {
	tabSize: uinteger;
	insertSpaces: boolean;
	trimTrailingWhitespace?: boolean;
	insertFinalNewline?: boolean;
	trimFinalNewlines?: boolean;
	// [key: string]: boolean | integer | string;
}

interface DocumentFormattingParams extends WorkDoneProgressParams {
	textDocument: TextDocumentIdentifier;
	options: FormattingOptions;
}
