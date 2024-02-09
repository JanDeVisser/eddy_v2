/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <unistd.h>

#define STATIC_ALLOCATOR
#include <allocate.h>
#include <eddy.h>
#include <json.h>
#include <lsp.h>
#include <process.h>
#include <sys/syslimits.h>

#define OR_NULL_WITH_ALIAS(T, A)                                                   \
    typedef struct {                                                               \
        bool is_not_null;                                                          \
        T    value;                                                                \
    } A##OrNull;                                                                   \
    OptionalJSONValue A##OrNull_encode(A##OrNull value)                            \
    {                                                                              \
        if (value.is_not_null) {                                                   \
            return A##_encode(value.value);                                        \
        } else {                                                                   \
            RETURN_VALUE(JSONValue, json_null());                                  \
        }                                                                          \
    }                                                                              \
    A##OrNull A##OrNull_decode(OptionalJSONValue json)                             \
    {                                                                              \
        assert(json.has_value);                                                    \
        if (json.value.type == JSON_TYPE_NULL) {                                   \
            return (A##OrNull) { 0 };                                              \
        } else {                                                                   \
            return (A##OrNull) { .is_not_null = true, .value = A##_decode(json) }; \
        }                                                                          \
    }
#define OR_NULL(T) OR_NULL_WITH_ALIAS(T, T)

#define OPTIONAL_JSON_WITH_ALIAS(T, A)                                             \
    OptionalJSONValue Optional##A##_encode(Optional##A value)                      \
    {                                                                              \
        if (value.has_value) {                                                     \
            return A##_encode(value.value);                                        \
        } else {                                                                   \
            RETURN_EMPTY(JSONValue);                                               \
        }                                                                          \
    }                                                                              \
    Optional##A Optional##A##_decode(OptionalJSONValue json)                       \
    {                                                                              \
        if (json.has_value) {                                                      \
            return (Optional##A) { .has_value = true, .value = A##_decode(json) }; \
        } else {                                                                   \
            return (Optional##A) { 0 };                                            \
        }                                                                          \
    }
#define OPTIONAL_JSON(T) OPTIONAL_JSON_WITH_ALIAS(T, T)

#define DA_JSON_WITH_ALIAS(T, A)                                     \
    OptionalJSONValue A##_encode(A value)                            \
    {                                                                \
        JSONValue ret = json_array();                                \
        for (size_t ix = 0; ix < value.size; ++ix) {                 \
            json_append(&ret, T##_encode(value.elements[ix]).value); \
        }                                                            \
        RETURN_VALUE(JSONValue, ret);                                \
    }                                                                \
    A A##_decode(OptionalJSONValue json)                             \
    {                                                                \
        assert(json.has_value);                                      \
        assert(json.value.type == JSON_TYPE_ARRAY);                  \
        A ret;                                                       \
        for (size_t ix = 0; ix < json_len(&json.value); ++ix) {      \
            OptionalJSONValue elem = json_at(&json.value, ix);       \
            da_append_##T(&ret, T##_decode(elem));                   \
        }                                                            \
        return ret;                                                  \
    }
#define DA_JSON(T) DA_JSON_WITH_ALIAS(T, T)

#define OptionalJSONValue_encode(V) (V)
#define OptionalJSONValue_decode(V) (V)
#define JSONValue_encode(V) (V)

typedef StringView         URI;
typedef OptionalStringView OptionalURI;
typedef StringView         DocumentUri;
typedef OptionalStringView OptionalDocumentUri;

#define Int_encode(V) ((OptionalJSONValue) { .has_value = true, .value = json_int(V) })
#define Int_decode(V) ({ assert(V.has_value); assert(V.value.type == JSON_TYPE_INT); json_int_value(V.value); })
#define Bool_encode(V) ((OptionalJSONValue) { .has_value = true, .value = json_bool(V) })
#define Bool_decode(V) ({ assert(V.has_value); assert(V.value.type == JSON_TYPE_BOOL); V.value.boolean; })
#define StringView_encode(V) ((OptionalJSONValue) { .has_value = true, .value = json_string(V) })
#define StringView_decode(V) ({ assert(V.has_value); assert(V.value.type == JSON_TYPE_STRING); V.value.string; })
#define URI_encode(V) ((OptionalJSONValue) { .has_value = true, .value = json_string(V) })
#define URI_decode(V) ((URI) StringView_decode(V))
#define DocumentUri_encode(V) ((OptionalJSONValue) { .has_value = true, .value = json_string(V) })
#define DocumentUri_decode(V) ((DocumentUri) StringView_decode(V))

OR_NULL_WITH_ALIAS(int, Int)
OPTIONAL_JSON(StringView)
OPTIONAL_JSON(URI)
OPTIONAL_JSON(DocumentUri)

typedef struct {
    int               id;
    char const       *method;
    OptionalJSONValue params;
} Request;

typedef struct {
    int               id;
    OptionalJSONValue result;
    OptionalJSONValue error;
} Response;

JSONValue request_encode(Request *request)
{
    JSONValue ret = json_object();
    json_set_cstr(&ret, "jsonrpc", "2.0");
    json_set_int(&ret, "id", request->id);
    json_set_cstr(&ret, "method", request->method);
    if (request->params.has_value) {
        json_set(&ret, "params", request->params.value);
    }
    return ret;
}

bool response_success(Response *response)
{
    return response->result.has_value;
}

bool response_error(Response *response)
{
    return response->error.has_value;
}

Response response_decode(JSONValue *json)
{
    Response ret = { 0 };
    ret.id = json_get_int(json, "id", 0);
    ret.result = json_get(json, "result");
    ret.error = json_get(json, "error");
    assert(response_success(&ret) ^ response_error(&ret));
    return ret;
}

#define EnumTraceValue(S)   \
    S(Off, "off")           \
    S(Messages, "messages") \
    S(Verbose, "verbose")

typedef enum {
#undef TraceValueValue
#define TraceValueValue(V, S) TraceValue##V,
    EnumTraceValue(TraceValueValue)
#undef TraceValueValue
} TraceValue;
OPTIONAL(TraceValue);

char const *TraceValue_to_string(TraceValue value)
{
    switch (value) {
#undef TraceValueValue
#define TraceValueValue(V, S) TraceValue##V : return S;
        EnumTraceValue(TraceValueValue)
#undef TraceValueValue
            default : UNREACHABLE();
    }
}

TraceValue TraceValue_parse(StringView s)
{
#undef TraceValueValue
#define TraceValueValue(V, S) \
    if (sv_eq_cstr(s, S))     \
        return TraceValue##V;
    EnumTraceValue(TraceValueValue)
#undef TraceValueValue
        UNREACHABLE();
}

OptionalJSONValue TraceValue_encode(TraceValue value)
{
    return StringView_encode(sv_from(TraceValue_to_string(value)));
}

TraceValue TraceValue_decode(OptionalJSONValue json)
{
    assert(json.has_value);
    assert(json.value.type == JSON_TYPE_STRING);
    return TraceValue_parse(json.value.string);
}

OPTIONAL_JSON(TraceValue)

#define StructClientCapabilities(S)

// clang-format off
typedef struct {
#undef ClientCapabilitiesProp
#define ClientCapabilitiesProp(N, T) T N;
    StructClientCapabilities(ClientCapabilitiesProp)
#undef ClientCapabilitiesProp
} ClientCapabilities;

OptionalJSONValue ClientCapabilities_encode(ClientCapabilities value)
{
    JSONValue ret = json_object();
#undef ClientCapabilitiesProp
#define ClientCapabilitiesProp(N, T) json_optional_set(&ret, #N, T## _encode(value.N));
    StructClientCapabilities(ClientCapabilitiesProp)
#undef ClientCapabilitiesProp
    RETURN_VALUE(JSONValue, ret);
}
// clang-format on

#define StructServerCapabilities(S) \
    S(experimental, OptionalJSONValue)

// clang-format off
typedef struct {
#undef ServerCapabilitiesProp
#define ServerCapabilitiesProp(N, T) T N;
    StructServerCapabilities(ServerCapabilitiesProp)
#undef ServerCapabilitiesProp
} ServerCapabilities;

ServerCapabilities ServerCapabilities_decode(OptionalJSONValue json)
{
    assert(json.has_value);
    assert(json.value.type == JSON_TYPE_OBJECT);
    ServerCapabilities value;
#undef ServerCapabilitiesProp
#define ServerCapabilitiesProp(N, T)                     \
    {                                                    \
        OptionalJSONValue v = json_get(&json.value, #N); \
        value.N = T## _decode(v);                         \
    }
    StructServerCapabilities(ServerCapabilitiesProp)
#undef ServerCapabilitiesProp
    return value;
}
// clang-format on

#define StructWorkspaceFolder(S) \
    S(uri, URI)                  \
    S(name, StringView)

// clang-format off
typedef struct {
#undef WorkspaceFolderProp
#define WorkspaceFolderProp(N, T) T N;
    StructWorkspaceFolder(WorkspaceFolderProp)
#undef WorkspaceFolderProp
} WorkspaceFolder;
// clang-format on

OptionalJSONValue WorkspaceFolder_encode(WorkspaceFolder value)
{
    JSONValue ret = json_object();
#undef WorkspaceFolderProp
#define WorkspaceFolderProp(N, T) json_optional_set(&ret, #N, T##_encode(value.N));
    StructWorkspaceFolder(WorkspaceFolderProp)
#undef WorkspaceFolderProp
        RETURN_VALUE(JSONValue, ret);
}

WorkspaceFolder WorkspaceFolder_decode(OptionalJSONValue json)
{
    assert(json.has_value);
    assert(json.value.type == JSON_TYPE_OBJECT);
    WorkspaceFolder value;
#undef WorkspaceFolderProp
#define WorkspaceFolderProp(N, T)                        \
    {                                                    \
        OptionalJSONValue v = json_get(&json.value, #N); \
        value.N = T##_decode(v);                         \
    }
    StructWorkspaceFolder(WorkspaceFolderProp)
#undef WorkspaceFolderProp
        return value;
}

OPTIONAL(WorkspaceFolder);
DA_WITH_NAME(WorkspaceFolder, WorkspaceFolders);
DA_JSON_WITH_ALIAS(WorkspaceFolder, WorkspaceFolders);
OR_NULL(WorkspaceFolders);
OPTIONAL(WorkspaceFoldersOrNull);
OPTIONAL_JSON(WorkspaceFoldersOrNull)

DA_IMPL(WorkspaceFolder);

#define StructInitializeParams(S)               \
    S(processId, IntOrNull)                     \
    S(locale, OptionalStringView)               \
    S(rootPath, OptionalStringView)             \
    S(rootUri, OptionalDocumentUri)             \
    S(initializationOptions, OptionalJSONValue) \
    S(capabilities, ClientCapabilities)         \
    S(trace, OptionalTraceValue)                \
    S(workspaceFolders, OptionalWorkspaceFoldersOrNull)

// clang-format off
typedef struct {
#undef InitializeParamsProp
#define InitializeParamsProp(N, T) T N;
    StructInitializeParams(InitializeParamsProp)
#undef InitializeParams
    struct {
        bool has_value;
        struct {
            StringView         name;
            OptionalStringView version;
        };
    } clientInfo;
} InitializeParams;

OptionalJSONValue InitializeParams_encode(InitializeParams value)
{
    JSONValue ret = json_object();
#undef InitializeParamsProp
#define InitializeParamsProp(N, T) json_optional_set(&ret, #N, T## _encode(value.N));
    StructInitializeParams(InitializeParamsProp)
#undef InitializeParamsProp
    if (value.clientInfo.has_value) {
        JSONValue clientInfo = json_object();
        json_set_string(&clientInfo, "name", value.clientInfo.name);
        json_optional_set(&clientInfo, "version", OptionalStringView_encode(value.clientInfo.version));
        json_set(&ret, "clientInfo", clientInfo);
    }
    RETURN_VALUE(JSONValue, ret);
}
// clang-format on

#define StructInitializeResult(S) \
    S(capabilities, ServerCapabilities)

// clang-format off
typedef struct {
#undef InitializeResultProp
#define InitializeResultProp(N, T) T N;
    StructInitializeResult(InitializeResultProp)
#undef InitializeResult
    struct {
        bool has_value;
        struct {
            StringView         name;
            OptionalStringView version;
        };
    } serverInfo;
} InitializeResult;

InitializeResult InitializeResult_decode(OptionalJSONValue json)
{
    assert(json.has_value)
    assert(json.value.type == JSON_TYPE_OBJECT);
    InitializeResult value = {0};
#undef InitializeResultProp
#define InitializeResultProp(N, T)                 \
    {                                              \
        OptionalJSONValue v = json_get(&json.value, #N); \
        value.N = T## _decode(v);                   \
    }
    StructInitializeResult(InitializeResultProp)
#undef InitializeResultProp
    OptionalJSONValue serverInfo = json_get(&json.value, "serverInfo");
    if ((value.serverInfo.has_value = serverInfo.has_value))
    {
        value.serverInfo.name = StringView_decode(json_get(&serverInfo.value, "name"));
        value.serverInfo.version = OptionalStringView_decode(json_get(&serverInfo.value, "version"));
    }
    return value;
}
// clang-format on

static Process           *lsp = NULL;
static ServerCapabilities server_capabilties;

Response lsp_message(char const *method, OptionalJSONValue params)
{
    static int id = 0;
    Request    req = { .id = ++id, method, OptionalJSONValue_empty() };
    req.params = params;
    assert(lsp);
    StringView  json = json_encode(request_encode(&req));
    char const *content_length = TextFormat("Content-Length: %zu\r\n\r\n", json.length + 2);
    write_pipe_write(&lsp->in, sv_from(content_length));
    write_pipe_write(&lsp->in, json);
    write_pipe_write(&lsp->in, sv_from("\r\n"));
    printf("-> LSP %.*s\n", SV_ARG(json));

    read_pipe_expect(&lsp->out);
    StringList lines = read_pipe_lines(&lsp->out);
    if (lines.size != 3) {
        fprintf(stderr, "LSP replied with %zu lines instead of 3\n", lines.size);
        return (Response) { 0 };
    }
    if (!sv_startswith(lines.strings[0], sv_from("Content-Length"))) {
        fprintf(stderr, "LSP replied with %.*s instead of 'Content-Length'\n", SV_ARG(lines.strings[0]));
        return (Response) { 0 };
    }
    if (!sv_empty(lines.strings[1])) {
        fprintf(stderr, "LSP replied with %.*s instead of empty line\n", SV_ARG(lines.strings[1]));
        return (Response) { 0 };
    }
    printf("LSP -> %.*s", SV_ARG(lines.strings[2]));
    ErrorOrJSONValue ret_maybe = json_decode(lines.strings[2]);
    if (ErrorOrJSONValue_is_error(ret_maybe)) {
        fprintf(stderr, "Could not JSON decode LSP reponse '%.*s': %s\n", SV_ARG(lines.strings[2]), Error_to_string(ret_maybe.error));
        return (Response) { 0 };
    }
    printf("LSP -> %.*s\n", SV_ARG(lines.strings[2]));
    Response response = response_decode(&ret_maybe.value);
    assert(response.id == req.id);
    return response;
}

void lsp_initialize()
{
    if (lsp != NULL) {
        return;
    }
    lsp = process_create(sv_from("clangd"));
    process_background(lsp);

    InitializeParams params = { 0 };
    params.processId = (IntOrNull) { .is_not_null = true, .value = getpid() };
    params.clientInfo.has_value = true;
    params.clientInfo.name = sv_from(EDDY_NAME);
    params.clientInfo.version = (OptionalStringView) { .has_value = true, .value = sv_from(EDDY_VERSION) };
    char prj[PATH_MAX + 8];
    memset(prj, '\0', PATH_MAX + 8);
    snprintf(prj, PATH_MAX + 7, "file://%.*s", SV_ARG(eddy.project_dir));
    params.rootUri = (OptionalStringView) { .has_value = true, .value = sv_from(prj) };

    OptionalJSONValue params_json = InitializeParams_encode(params);
    Response          response = lsp_message("initialize", params_json);
    if (response_success(&response)) {
        InitializeResult result = InitializeResult_decode(response.result);
        if (result.serverInfo.has_value) {
            fprintf(stderr, "LSP server name: %.*s\n", SV_ARG(result.serverInfo.name));
            if (result.serverInfo.version.has_value) {
                fprintf(stderr, "LSP server version: %.*s\n", SV_ARG(result.serverInfo.version.value));
            }
        }
        server_capabilties = result.capabilities;
    }
}

void lsp_on_open(StringView name)
{
    lsp_initialize();
}
