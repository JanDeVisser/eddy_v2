/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/CompletionList.h>

DA_IMPL(CompletionList)

OptionalJSONValue CompletionLists_encode(CompletionLists value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, CompletionList_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalCompletionLists CompletionLists_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(CompletionLists);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    CompletionLists ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue      elem = json_at(&json.value, ix);
        OptionalCompletionList val = CompletionList_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(CompletionLists);
        }
        da_append_CompletionList(&ret, val.value);
    }
    RETURN_VALUE(CompletionLists, ret);
}

OptionalCompletionList CompletionList_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(CompletionList);
    }
    CompletionList value = { 0 };
    {
        OptionalJSONValue v0 = json_get(&json.value, "isIncomplete");
        value.isIncomplete = FORWARD_OPTIONAL(Bool, CompletionList, Bool_decode(v0));
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "itemDefaults");
        if (v0.has_value) {
            value.itemDefaults.has_value = true;
            {
                OptionalJSONValue v1 = json_get(&v0.value, "commitCharacters");
                value.itemDefaults.commitCharacters = StringViews_decode(v1);
            }

            {
                OptionalJSONValue v1 = json_get(&v0.value, "editRange");
                if (v1.has_value) {
                    value.itemDefaults.editRange.has_value = true;
                    while (true) {
                        {
                            OptionalRange decoded = Range_decode(v1);
                            if (decoded.has_value) {
                                value.itemDefaults.editRange.tag = 0;
                                value.itemDefaults.editRange._0 = decoded.value;
                                break;
                            }
                        }
                        {
                            bool              decoded2 = false;
                            OptionalJSONValue v3 = { 0 };
                            do {
                                v3 = json_get(&v1.value, "insert");
                                OptionalRange opt2_1_insert = Range_decode(v3);
                                if (!opt2_1_insert.has_value) {
                                    break;
                                }
                                value.itemDefaults.editRange._1.insert = opt2_1_insert.value;
                                v3 = json_get(&v1.value, "replace");
                                OptionalRange opt2_1_replace = Range_decode(v3);
                                if (!opt2_1_replace.has_value) {
                                    break;
                                }
                                value.itemDefaults.editRange._1.replace = opt2_1_replace.value;
                                decoded2 = true;
                            } while (false);
                            if (decoded2)
                                break;
                        }
                        RETURN_EMPTY(CompletionList);
                    }
                }
            }

            {
                OptionalJSONValue v1 = json_get(&v0.value, "insertTextFormat");
                value.itemDefaults.insertTextFormat = InsertTextFormat_decode(v1);
            }

            {
                OptionalJSONValue v1 = json_get(&v0.value, "insertTextMode");
                value.itemDefaults.insertTextMode = InsertTextMode_decode(v1);
            }

            {
                OptionalJSONValue v1 = json_get(&v0.value, "data");
                value.itemDefaults.data = JSONValue_decode(v1);
            }
        }
    }
    {
        OptionalJSONValue v0 = json_get(&json.value, "items");
        value.items = FORWARD_OPTIONAL(CompletionItems, CompletionList, CompletionItems_decode(v0));
    }
    RETURN_VALUE(CompletionList, value);
}

OptionalJSONValue CompletionList_encode(CompletionList value)
{
    JSONValue v1 = json_object();
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = Bool_encode(value.isIncomplete);
        json_optional_set(&v1, "isIncomplete", _encoded_maybe);
    }
    if (value.itemDefaults.has_value) {
        JSONValue v2 = json_object();
        {
            OptionalJSONValue _encoded_maybe = { 0 };
            if (value.itemDefaults.commitCharacters.has_value) {
                _encoded_maybe = StringViews_encode(value.itemDefaults.commitCharacters.value);
            }
            json_optional_set(&v2, "commitCharacters", _encoded_maybe);
        }
        if (value.itemDefaults.editRange.has_value) {
            JSONValue v3 = { 0 };
            switch (value.itemDefaults.editRange.tag) {
            case 0:
                v3 = MUST_OPTIONAL(JSONValue, Range_encode(value.itemDefaults.editRange._0));

                break;
            case 1: {
                v3 = json_object();
                {
                    OptionalJSONValue _encoded_maybe = { 0 };
                    _encoded_maybe = Range_encode(value.itemDefaults.editRange._1.insert);
                    json_optional_set(&v3, "insert", _encoded_maybe);
                }
                {
                    OptionalJSONValue _encoded_maybe = { 0 };
                    _encoded_maybe = Range_encode(value.itemDefaults.editRange._1.replace);
                    json_optional_set(&v3, "replace", _encoded_maybe);
                }
            } break;
            default:
                UNREACHABLE();
            }

            json_set(&v2, "editRange", v3);
        }

        {
            OptionalJSONValue _encoded_maybe = { 0 };
            if (value.itemDefaults.insertTextFormat.has_value) {
                _encoded_maybe = InsertTextFormat_encode(value.itemDefaults.insertTextFormat.value);
            }
            json_optional_set(&v2, "insertTextFormat", _encoded_maybe);
        }
        {
            OptionalJSONValue _encoded_maybe = { 0 };
            if (value.itemDefaults.insertTextMode.has_value) {
                _encoded_maybe = InsertTextMode_encode(value.itemDefaults.insertTextMode.value);
            }
            json_optional_set(&v2, "insertTextMode", _encoded_maybe);
        }
        {
            OptionalJSONValue _encoded_maybe = { 0 };
            if (value.itemDefaults.data.has_value) {
                _encoded_maybe = JSONValue_encode(value.itemDefaults.data.value);
            }
            json_optional_set(&v2, "data", _encoded_maybe);
        }
        json_set(&v1, "itemDefaults", v2);
    }
    {
        OptionalJSONValue _encoded_maybe = { 0 };
        _encoded_maybe = CompletionItems_encode(value.items);
        json_optional_set(&v1, "items", _encoded_maybe);
    }
    RETURN_VALUE(JSONValue, v1);
}
