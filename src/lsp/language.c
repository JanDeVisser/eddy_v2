/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */
 
// clang-format off 

#include <lsp/language.h>

OptionalJSONValue FormattingOptions_encode(FormattingOptions value)
{
    JSONValue v4 = json_object();
    json_optional_set(&v4, "tabSize", Int_encode(value.tabSize));
    json_optional_set(&v4, "insertSpaces", Bool_encode(value.insertSpaces));
    json_optional_set(&v4, "trimTrailingWhitespace", OptionalBool_encode(value.trimTrailingWhitespace));
    json_optional_set(&v4, "insertFinalNewline", OptionalBool_encode(value.insertFinalNewline));
    json_optional_set(&v4, "trimFinalNewlines", OptionalBool_encode(value.trimFinalNewlines));
    RETURN_VALUE(JSONValue, v4);
}

OptionalJSONValue DocumentFormattingParams_encode(DocumentFormattingParams value)
{
    JSONValue v4 = json_object();
    json_optional_set(&v4, "textDocument", TextDocumentIdentifier_encode(value.textDocument));
    json_optional_set(&v4, "options", FormattingOptions_encode(value.options));
    RETURN_VALUE(JSONValue, v4);
}

// clang-format on

