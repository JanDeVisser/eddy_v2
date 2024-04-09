/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/CompletionItem.h>

DA_IMPL(CompletionItem)

OptionalJSONValue CompletionItems_encode(CompletionItems value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, CompletionItem_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalCompletionItems CompletionItems_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(CompletionItems);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    CompletionItems ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue      elem = json_at(&json.value, ix);
        OptionalCompletionItem val = CompletionItem_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(CompletionItems);
        }
        da_append_CompletionItem(&ret, val.value);
    }
    RETURN_VALUE(CompletionItems, ret);
}

OptionalCompletionItem CompletionItem_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(CompletionItem);
    }
    CompletionItem value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "label");
        value.label = FORWARD_OPTIONAL(StringView, CompletionItem, StringView_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "labelDetails");
        value.labelDetails = CompletionItemLabelDetails_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "kind");
        value.kind = CompletionItemKind_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "tags");
        value.tags = CompletionItemTags_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "detail");
        value.detail = StringView_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "documentation");
        if (v0.has_value) {
            value.documentation.has_value = true;
            while (true) {
                {
                    OptionalStringView decoded = StringView_decode(v0);
                    if (decoded.has_value) {
                        value.documentation.tag = 0;
                        value.documentation._0 = decoded.value;
                        break;
                    }
                }
                {
                    OptionalMarkupContent decoded = MarkupContent_decode(v0);
                    if (decoded.has_value) {
                        value.documentation.tag = 1;
                        value.documentation._1 = decoded.value;
                        break;
                    }
                }
                RETURN_EMPTY(CompletionItem);
            }
        }
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "deprecated");
        value.deprecated = Bool_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "preselect");
        value.preselect = Bool_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "sortText");
        value.sortText = StringView_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "filterText");
        value.filterText = StringView_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "insertText");
        value.insertText = StringView_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "insertTextFormat");
        value.insertTextFormat = InsertTextFormat_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "insertTextMode");
        value.insertTextMode = InsertTextMode_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "textEdit");
        if (v0.has_value) {
            value.textEdit.has_value = true;
            while (true) {
                {
                    OptionalTextEdit decoded = TextEdit_decode(v0);
                    if (decoded.has_value) {
                        value.textEdit.tag = 0;
                        value.textEdit._0 = decoded.value;
                        break;
                    }
                }
                {
                    OptionalInsertReplaceEdit decoded = InsertReplaceEdit_decode(v0);
                    if (decoded.has_value) {
                        value.textEdit.tag = 1;
                        value.textEdit._1 = decoded.value;
                        break;
                    }
                }
                RETURN_EMPTY(CompletionItem);
            }
        }
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "textEditText");
        value.textEditText = StringView_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "additionalTextEdits");
        value.additionalTextEdits = TextEdits_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "commitCharacters");
        value.commitCharacters = StringViews_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "command");
        value.command = LSPCommand_decode(v0);
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "data");
        value.data = JSONValue_decode(v0);
    }
    RETURN_VALUE(CompletionItem, value);
}

OptionalJSONValue CompletionItem_encode(CompletionItem value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = StringView_encode(value.label);
        json_optional_set(&v1, "label", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.labelDetails.has_value) {
            _encoded_maybe = CompletionItemLabelDetails_encode(value.labelDetails.value);
        }
        json_optional_set(&v1, "labelDetails", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.kind.has_value) {
            _encoded_maybe = CompletionItemKind_encode(value.kind.value);
        }
        json_optional_set(&v1, "kind", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.tags.has_value) {
            _encoded_maybe = CompletionItemTags_encode(value.tags.value);
        }
        json_optional_set(&v1, "tags", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.detail.has_value) {
            _encoded_maybe = StringView_encode(value.detail.value);
        }
        json_optional_set(&v1, "detail", _encoded_maybe);
    }
    if (value.documentation.has_value) {
        JSONValue v2 = { 0 };
        switch (value.documentation.tag) {
        case 0:
            v2 = MUST_OPTIONAL(JSONValue, StringView_encode(value.documentation._0));

            break;
        case 1:
            v2 = MUST_OPTIONAL(JSONValue, MarkupContent_encode(value.documentation._1));

            break;
        default:
            UNREACHABLE();
        }

        json_set(&v1, "documentation", v2);
    }

    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.deprecated.has_value) {
            _encoded_maybe = Bool_encode(value.deprecated.value);
        }
        json_optional_set(&v1, "deprecated", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.preselect.has_value) {
            _encoded_maybe = Bool_encode(value.preselect.value);
        }
        json_optional_set(&v1, "preselect", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.sortText.has_value) {
            _encoded_maybe = StringView_encode(value.sortText.value);
        }
        json_optional_set(&v1, "sortText", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.filterText.has_value) {
            _encoded_maybe = StringView_encode(value.filterText.value);
        }
        json_optional_set(&v1, "filterText", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.insertText.has_value) {
            _encoded_maybe = StringView_encode(value.insertText.value);
        }
        json_optional_set(&v1, "insertText", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.insertTextFormat.has_value) {
            _encoded_maybe = InsertTextFormat_encode(value.insertTextFormat.value);
        }
        json_optional_set(&v1, "insertTextFormat", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.insertTextMode.has_value) {
            _encoded_maybe = InsertTextMode_encode(value.insertTextMode.value);
        }
        json_optional_set(&v1, "insertTextMode", _encoded_maybe);
    }
    if (value.textEdit.has_value) {
        JSONValue v2 = { 0 };
        switch (value.textEdit.tag) {
        case 0:
            v2 = MUST_OPTIONAL(JSONValue, TextEdit_encode(value.textEdit._0));

            break;
        case 1:
            v2 = MUST_OPTIONAL(JSONValue, InsertReplaceEdit_encode(value.textEdit._1));

            break;
        default:
            UNREACHABLE();
        }

        json_set(&v1, "textEdit", v2);
    }

    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.textEditText.has_value) {
            _encoded_maybe = StringView_encode(value.textEditText.value);
        }
        json_optional_set(&v1, "textEditText", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.additionalTextEdits.has_value) {
            _encoded_maybe = TextEdits_encode(value.additionalTextEdits.value);
        }
        json_optional_set(&v1, "additionalTextEdits", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.commitCharacters.has_value) {
            _encoded_maybe = StringViews_encode(value.commitCharacters.value);
        }
        json_optional_set(&v1, "commitCharacters", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.command.has_value) {
            _encoded_maybe = LSPCommand_encode(value.command.value);
        }
        json_optional_set(&v1, "command", _encoded_maybe);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        if (value.data.has_value) {
            _encoded_maybe = JSONValue_encode(value.data.value);
        }
        json_optional_set(&v1, "data", _encoded_maybe);
    }
    RETURN_VALUE(JSONValue, v1);
}
