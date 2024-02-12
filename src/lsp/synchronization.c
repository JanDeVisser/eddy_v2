/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */
 
// clang-format off 

#include <lsp/synchronization.h>

OptionalJSONValue DidOpenTextDocumentParams_encode(DidOpenTextDocumentParams value)
{
    JSONValue v4 = json_object();
    json_optional_set(&v4, "textDocument", TextDocumentItem_encode(value.textDocument));
    RETURN_VALUE(JSONValue, v4);
}

// clang-format on

