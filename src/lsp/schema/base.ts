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

export type MarkupKind = string;
export namespace MarkupKind {
	export const PlainText: MarkupKind = "plaintext";
	export const Markdown: MarkupKind = 'markdown';
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

interface LSPCommand {
	title: string;
	command: string;
	arguments?: LSPAny[];
}

export interface MarkupContent {
	kind: MarkupKind;
	value: string;
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

export type CompletionTriggerKind = integer;
export namespace CompletionTriggerKind {
	export const Invoked: CompletionTriggerKind = 1;
	export const TriggerCharacter: CompletionTriggerKind = 2;
	export const TriggerForIncompleteCompletions: CompletionTriggerKind = 3;
}

export interface CompletionContext {
	triggerKind: CompletionTriggerKind;
	triggerCharacter?: string;
}

export interface CompletionParams extends TextDocumentPositionParams, WorkDoneProgressParams {
	context?: CompletionContext;
}

export type InsertTextFormat = integer;
export namespace InsertTextFormat {
	export const PlainText: InsertTextFormat = 1;
	export const Snippet: InsertTextFormat = 2;
}

export type CompletionItemTag = integer;
export namespace CompletionItemTag {
	export const Deprecated: CompletionItemTag = 1;
}

export interface InsertReplaceEdit {
	newText: string;
	insert: Range;
	replace: Range;
}

export type InsertTextMode = integer;
export namespace InsertTextMode {
	export const asIs: InsertTextMode = 1;
	export const adjustIndentation: InsertTextMode = 2;
}


export interface CompletionItemLabelDetails {
	detail?: string;
	description?: string;
}

export type CompletionItemKind = integer;
export namespace CompletionItemKind {
	export const Text: CompletionItemKind = 1;
	export const Method: CompletionItemKind = 2;
	export const Function: CompletionItemKind = 3;
	export const Constructor: CompletionItemKind = 4;
	export const Field: CompletionItemKind = 5;
	export const Variable: CompletionItemKind = 6;
	export const Class: CompletionItemKind = 7;
	export const Interface: CompletionItemKind = 8;
	export const Module: CompletionItemKind = 9;
	export const Property: CompletionItemKind = 10;
	export const Unit: CompletionItemKind = 11;
	export const Value: CompletionItemKind = 12;
	export const Enum: CompletionItemKind = 13;
	export const Keyword: CompletionItemKind = 14;
	export const Snippet: CompletionItemKind = 15;
	export const Color: CompletionItemKind = 16;
	export const File: CompletionItemKind = 17;
	export const Reference: CompletionItemKind = 18;
	export const Folder: CompletionItemKind = 19;
	export const EnumMember: CompletionItemKind = 20;
	export const Constant: CompletionItemKind = 21;
	export const Struct: CompletionItemKind = 22;
	export const Event: CompletionItemKind = 23;
	export const Operator: CompletionItemKind = 24;
	export const TypeParameter: CompletionItemKind = 25;
}

export interface CompletionItem {
	label: string;
	labelDetails?: CompletionItemLabelDetails;
	kind?: CompletionItemKind;
	tags?: CompletionItemTag[];
	detail?: string;
	documentation?: string | MarkupContent;
	deprecated?: boolean;
	preselect?: boolean;
	sortText?: string;
	filterText?: string;
	insertText?: string;
	insertTextFormat?: InsertTextFormat;
	insertTextMode?: InsertTextMode;
	textEdit?: TextEdit | InsertReplaceEdit;
	textEditText?: string;
	additionalTextEdits?: TextEdit[];
	commitCharacters?: string[];
	command?: LSPCommand;
	data?: LSPAny;
}

export interface CompletionList {
	isIncomplete: boolean;
	itemDefaults?: {
		commitCharacters?: string[];
		editRange?: Range | {
			insert: Range;
			replace: Range;
		};
		insertTextFormat?: InsertTextFormat;
		insertTextMode?: InsertTextMode;
		data?: LSPAny;
	};
	items: CompletionItem[];
}

export interface PublishDiagnosticsClientCapabilities {
	relatedInformation?: boolean;
	tagSupport?: {
		valueSet: DiagnosticTag[];
	};
	versionSupport?: boolean;
	codeDescriptionSupport?: boolean;
	dataSupport?: boolean;
}

export interface Diagnostic {
	range: Range;
	severity?: DiagnosticSeverity;
	code?: integer | string;
	codeDescription?: CodeDescription;
	source?: string;
	message: string;
	tags?: DiagnosticTag[];
	relatedInformation?: DiagnosticRelatedInformation[];
	data?: LSPAny;
}

export type DiagnosticSeverity = integer;
export namespace DiagnosticSeverity {
	export const Error: integer = 1;
	export const Warning: integer = 2;
	export const Information: integer = 3;
	export const Hint: integer = 4;
}


export type DiagnosticTag = integer;
export namespace DiagnosticTag {
	export const Unnecessary: integer = 1;
	export const Deprecated: integer = 2;
}


export interface DiagnosticRelatedInformation {
	location: Location;
	message: string;
}

export interface CodeDescription {
	href: URI;
}

interface PublishDiagnosticsParams {
	uri: DocumentUri;
	version?: integer;
	diagnostics: Diagnostic[];
}
