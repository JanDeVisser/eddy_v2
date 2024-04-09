/**
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * THIS IS GENERATED CODE. DO NOT MODIFY.
 */

#include <lsp/schema/WorkDoneProgressParams.h>

DA_IMPL(WorkDoneProgressParams)

OptionalJSONValue WorkDoneProgressParamss_encode(WorkDoneProgressParamss value)
{
    JSONValue ret = json_array();
    for (size_t ix = 0; ix < value.size; ++ix) {
        json_append(&ret, WorkDoneProgressParams_encode(value.elements[ix]).value);
    }
    RETURN_VALUE(JSONValue, ret);
}

OptionalWorkDoneProgressParamss WorkDoneProgressParamss_decode(OptionalJSONValue json)
{
    if (!json.has_value) {
        RETURN_EMPTY(WorkDoneProgressParamss);
    }
    assert(json.value.type == JSON_TYPE_ARRAY);
    WorkDoneProgressParamss ret = { 0 };
    for (size_t ix = 0; ix < json_len(&json.value); ++ix) {
        OptionalJSONValue              elem = json_at(&json.value, ix);
        OptionalWorkDoneProgressParams val = WorkDoneProgressParams_decode(elem);
        if (!val.has_value) {
            RETURN_EMPTY(WorkDoneProgressParamss);
        }
        da_append_WorkDoneProgressParams(&ret, val.value);
    }
    RETURN_VALUE(WorkDoneProgressParamss, ret);
}

OptionalWorkDoneProgressParams WorkDoneProgressParams_decode(OptionalJSONValue json)
{
    if (!json.has_value || json.value.type != JSON_TYPE_OBJECT) {
        RETURN_EMPTY(WorkDoneProgressParams);
    }
    WorkDoneProgressParams value = {};
    RETURN_VALUE(WorkDoneProgressParams, value);
}

OptionalJSONValue WorkDoneProgressParams_encode(WorkDoneProgressParams value)
{
    JSONValue v1 = json_object();
    RETURN_VALUE(JSONValue, v1);
}
