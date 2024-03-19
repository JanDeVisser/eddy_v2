/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h>

#include <ctype.h>
#include <json.h>

DA_IMPL(JSONValue);
DA_IMPL(JSONNVPair);

typedef struct {
    StringBuilder sb;
    StringBuilder escaped;
    int           indent;
} JSONEncoder;

char const *JSONType_name(JSONType type)
{
    switch (type) {
#undef S
#define S(T)            \
    case JSON_TYPE_##T: \
        return #T;
        JSONTYPES(S)
#undef S
    default:
        UNREACHABLE();
    }
}

void json_encode_to_builder(JSONValue *value, JSONEncoder *encoder)
{
    switch (value->type) {
    case JSON_TYPE_OBJECT: {
        sb_append_cstr(&encoder->sb, "{\n");
        for (size_t ix = 0; ix < value->object.size; ix++) {
            if (ix > 0) {
                sb_append_cstr(&encoder->sb, ",\n");
            }
            JSONNVPair *nvp = da_element_JSONNVPair(&value->object, ix);
            encoder->indent += 4;
            sb_printf(&encoder->sb, "%*s\"%.*s\": ", encoder->indent, "", SV_ARG(nvp->name));
            json_encode_to_builder(&nvp->value, encoder);
            encoder->indent -= 4;
        }
        sb_printf(&encoder->sb, "\n%*c", encoder->indent + 1, '}');
    } break;
    case JSON_TYPE_ARRAY: {
        sb_append_cstr(&encoder->sb, "[\n");
        for (int ix = 0; ix < value->array.size; ix++) {
            if (ix > 0) {
                sb_append_cstr(&encoder->sb, ",\n");
            }
            JSONValue *elem = da_element_JSONValue(&value->array, ix);
            encoder->indent += 4;
            sb_printf(&encoder->sb, "%*s", encoder->indent, "");
            json_encode_to_builder(elem, encoder);
            encoder->indent -= 4;
        }
        sb_printf(&encoder->sb, "\n%*c", encoder->indent + 1, ']');
    } break;
    case JSON_TYPE_STRING:
        encoder->escaped.view.length = 0;
        encoder->escaped = sb_copy_sv(value->string);
        sb_replace_all(&encoder->escaped, sv_from("\\"), sv_from("\\\\"));
        sb_replace_all(&encoder->escaped, sv_from("\n"), sv_from("\\n"));
        sb_replace_all(&encoder->escaped, sv_from("\r"), sv_from("\\r"));
        sb_replace_all(&encoder->escaped, sv_from("\t"), sv_from("\\t"));
        sb_replace_all(&encoder->escaped, sv_from("\""), sv_from("\\\""));
        sb_printf(&encoder->sb, "\"%.*s\"", SV_ARG(encoder->escaped.view));
        break;
    case JSON_TYPE_DOUBLE:
        sb_printf(&encoder->sb, "%f", value->double_number);
        break;
    case JSON_TYPE_INT:
        sb_append_integer(&encoder->sb, value->int_number);
        break;
    case JSON_TYPE_BOOLEAN:
        sb_append_cstr(&encoder->sb, (value->boolean) ? "true" : "false");
        break;
    case JSON_TYPE_NULL:
        sb_append_cstr(&encoder->sb, "null");
        break;
    default:
        fatal("Invalid JSON type %d", value->type);
    }
}

StringView json_to_string(JSONValue value)
{
    switch (value.type) {
    case JSON_TYPE_OBJECT:
    case JSON_TYPE_ARRAY: {
        return json_encode(value);
    } break;
    case JSON_TYPE_STRING:
        return sv_copy(value.string);
    case JSON_TYPE_DOUBLE:
        return sv_printf("%f", value.double_number);
        break;
    case JSON_TYPE_INT:
        return sv_render_integer(value.int_number);
        break;
    case JSON_TYPE_BOOLEAN:
        if (value.boolean) {
            return sv_from("true");
        }
        return sv_from("false");
    case JSON_TYPE_NULL:
        return sv_from("null");
    default:
        UNREACHABLE();
    }
}

JSONValue json_object(void)
{
    JSONValue result = { 0 };
    result.type = JSON_TYPE_OBJECT;
    return result;
}

JSONValue json_array(void)
{
    JSONValue result = { 0 };
    result.type = JSON_TYPE_ARRAY;
    return result;
}

JSONValue json_null(void)
{
    return (JSONValue) { .type = JSON_TYPE_NULL };
}

JSONValue json_string(StringView sv)
{
    return (JSONValue) { .type = JSON_TYPE_STRING, .string = sv_copy(sv) };
}

JSONValue json_number(double number)
{
    return (JSONValue) { .type = JSON_TYPE_DOUBLE, .double_number = number };
}

JSONValue json_int(int number)
{
    return (JSONValue) { .type = JSON_TYPE_INT, .int_number = i32(number) };
}

JSONValue json_integer(Integer number)
{
    return (JSONValue) { .type = JSON_TYPE_INT, .int_number = number };
}

int json_int_value(JSONValue value)
{
    assert(value.type == JSON_TYPE_INT);
    Integer as_i32 = MUST_OPTIONAL(Integer, integer_coerce_to(value.int_number, I32));
    return as_i32.i32;
}

JSONValue json_bool(bool value)
{
    return (JSONValue) { .type = JSON_TYPE_BOOLEAN, .boolean = value };
}

void json_free(JSONValue value)
{
    switch (value.type) {
    case JSON_TYPE_OBJECT: {
        for (size_t ix = 0; ix < value.object.size; ++ix) {
            JSONNVPair pair = *(da_element_JSONNVPair(&value.object, ix));
            sv_free(pair.name);
            json_free(pair.value);
        }
        da_free_JSONNVPair(&value.object);
    } break;
    case JSON_TYPE_ARRAY: {
        for (size_t ix = 0; ix < value.array.size; ++ix) {
            JSONValue v = *(da_element_JSONValue(&value.array, ix));
            json_free(v);
        }
        da_free_JSONValue(&value.array);
    } break;
    case JSON_TYPE_STRING:
        sv_free(value.string);
        break;
    default:
        break;
    }
}

JSONValue json_move(JSONValue *from)
{
    JSONValue dest = *from;
    switch (from->type) {
    case JSON_TYPE_OBJECT:
        from->object = (JSONNVPairs) { 0 };
        break;
    case JSON_TYPE_STRING:
        from->string = (StringView) { 0 };
        break;
    default:
        break;
    }
    from->type = JSON_TYPE_NULL;
    return dest;
}

JSONValue json_copy(JSONValue value)
{
    JSONValue ret = value;
    switch (value.type) {
    case JSON_TYPE_OBJECT: {
        ret.object = (JSONNVPairs) { 0 };
        for (size_t ix = 0; ix < value.object.size; ++ix) {
            JSONNVPair pair = *(da_element_JSONNVPair(&value.object, ix));
            json_set(&ret, sv_cstr(pair.name), json_copy(pair.value));
        }
    } break;
    case JSON_TYPE_ARRAY: {
        ret.array = (JSONValues) { 0 };
        for (size_t ix = 0; ix < value.array.size; ++ix) {
            JSONValue v = *(da_element_JSONValue(&value.array, ix));
            json_append(&ret, json_copy(v));
        }
    } break;
    case JSON_TYPE_STRING:
        ret.string = sv_copy(value.string);
        break;
    default:
        break;
    }
    return ret;
}

int json_compare(JSONValue a, JSONValue b)
{
    if (a.type != b.type) {
        return a.type - b.type;
    }
    switch (a.type) {
    case JSON_TYPE_OBJECT: {
        if (a.object.size != b.object.size) {
            return a.object.size - b.object.size;
        }
        for (size_t ix = 0; ix < a.object.size; ++ix) {
            JSONNVPair *ap = da_element_JSONNVPair(&a.object, ix);
            JSONNVPair *bp = da_element_JSONNVPair(&b.object, ix);
            int         cmp = sv_cmp(ap->name, bp->name);
            if (cmp != 0) {
                return cmp;
            }
            cmp = json_compare(ap->value, bp->value);
            if (cmp != 0) {
                return cmp;
            }
        }
    } break;
    case JSON_TYPE_ARRAY: {
        if (a.array.size != b.array.size) {
            return a.array.size - b.array.size;
        }
        for (size_t ix = 0; ix < a.array.size; ++ix) {
            JSONValue *av = da_element_JSONValue(&a.array, ix);
            JSONValue *bv = da_element_JSONValue(&b.array, ix);
            int        cmp = json_compare(*av, *bv);
            if (cmp != 0) {
                return cmp;
            }
        }
    } break;
    case JSON_TYPE_STRING:
        return sv_cmp(a.string, b.string);
    case JSON_TYPE_INT: {
        Integer cmp = integer_subtract(a.int_number, b.int_number);
        Integer ret = MUST_OPTIONAL(Integer, integer_coerce_to(cmp, I32));
        return ret.i32;
    }
    case JSON_TYPE_DOUBLE:
        return (a.double_number > b.double_number) - (a.double_number < b.double_number);
    case JSON_TYPE_BOOLEAN:
        return a.boolean - b.boolean;
    case JSON_TYPE_NULL:
        return 0;
    default:
        UNREACHABLE();
    }
    return 0;
}

void json_append(JSONValue *array, JSONValue elem)
{
    assert(array->type == JSON_TYPE_ARRAY);
    da_append_JSONValue(&array->array, elem);
}

void json_add(JSONValue *array, JSONValue elem)
{
    OptionalInt ix = json_find(array, elem);
    if (!ix.has_value) {
        json_append(array, elem);
    }
}

OptionalInt json_find(JSONValue *array, JSONValue elem)
{
    assert(array->type == JSON_TYPE_ARRAY);
    for (int ix = 0; ix < array->array.size; ++ix) {
        JSONValue *v = da_element_JSONValue(&array->array, ix);
        if (json_compare(*v, elem) == 0) {
            RETURN_VALUE(Int, ix);
        }
    }
    RETURN_EMPTY(Int);
}

OptionalJSONValue json_at(JSONValue *array, size_t index)
{
    assert(array->type == JSON_TYPE_ARRAY);
    if (index < array->array.size) {
        RETURN_VALUE(JSONValue, *da_element_JSONValue(&array->array, index));
    }
    RETURN_EMPTY(JSONValue);
}

JSONValue *json_at_ref(JSONValue *array, size_t index)
{
    assert(array->type == JSON_TYPE_ARRAY);
    if (index < array->array.size) {
        return da_element_JSONValue(&array->array, index);
    }
    return NULL;
}

size_t json_len(JSONValue *array)
{
    assert(array->type == JSON_TYPE_ARRAY || array->type == JSON_TYPE_OBJECT);
    if (array->type == JSON_TYPE_ARRAY) {
        return array->array.size;
    }
    return array->object.size;
}

static void _json_add_nvp(JSONValue *obj, StringView attr, JSONValue value)
{
    assert(obj->type == JSON_TYPE_OBJECT);
    for (size_t ix = 0; ix < obj->object.size; ++ix) {
        JSONNVPair *p = da_element_JSONNVPair(&obj->object, ix);
        if (sv_eq(attr, p->name)) {
            json_free(p->value);
            p->value = value;
            return;
        }
    }
    JSONNVPair pair = { .name = sv_copy(attr), .value = value };
    da_append_JSONNVPair(&obj->object, pair);
}

void json_set(JSONValue *obj, char const *attr, JSONValue elem)
{
    _json_add_nvp(obj, sv_from(attr), elem);
}

void json_optional_set(JSONValue *obj, char const *attr, OptionalJSONValue elem)
{
    if (elem.has_value) {
        _json_add_nvp(obj, sv_from(attr), elem.value);
    }
}

void json_set_sv(JSONValue *value, StringView attr, JSONValue elem)
{
    _json_add_nvp(value, attr, elem);
}

void json_set_cstr(JSONValue *value, char const *attr, char const *s)
{
    _json_add_nvp(value, sv_from(attr), json_string(sv_from(s)));
}

void json_set_string(JSONValue *value, char const *attr, StringView sv)
{
    _json_add_nvp(value, sv_from(attr), json_string(sv_copy(sv)));
}

void json_set_int(JSONValue *value, char const *attr, int i)
{
    _json_add_nvp(value, sv_from(attr), json_int(i));
}

void json_set_int_sv(JSONValue *value, StringView attr, int i)
{
    _json_add_nvp(value, attr, json_int(i));
}

bool json_has(JSONValue *value, char const *attr)
{
    return json_has_sv(value, sv_from(attr));
}

bool json_has_sv(JSONValue *value, StringView attr)
{
    assert(value->type == JSON_TYPE_OBJECT);
    for (size_t ix = 0; ix < value->object.size; ++ix) {
        JSONNVPair const *pair = da_element_JSONNVPair(&value->object, ix);
        if (sv_eq(pair->name, attr)) {
            return true;
        }
    }
    return false;
}

void json_delete(JSONValue *value, char const *attr)
{
    json_delete_sv(value, sv_from(attr));
}

void json_delete_sv(JSONValue *value, StringView attr)
{
    assert(value->type == JSON_TYPE_OBJECT);
    for (size_t ix = 0; ix < value->object.size; ++ix) {
        JSONNVPair *pair = da_element_JSONNVPair(&value->object, ix);
        if (sv_eq(pair->name, attr)) {
            if (ix < value->object.size - 1) {
                memmove(pair, da_element_JSONNVPair(&value->object, ix + 1), sizeof(JSONNVPair));
            }
            --value->object.size;
            return;
        }
    }
}

OptionalJSONValue json_entry_at(JSONValue *value, int ix)
{
    assert(value->type == JSON_TYPE_OBJECT);
    if (ix < value->array.size) {
        JSONNVPair *pair = da_element_JSONNVPair(&value->object, ix);
        JSONValue   ret = json_array();
        json_append(&ret, json_string(pair->name));
        json_append(&ret, pair->value);
        RETURN_VALUE(JSONValue, ret);
    }
    RETURN_EMPTY(JSONValue);
}

OptionalJSONValue json_get(JSONValue *value, char const *attr)
{
    return json_get_sv(value, sv_from(attr));
}

OptionalJSONValue json_get_sv(JSONValue *value, StringView attr)
{
    assert(value->type == JSON_TYPE_OBJECT);
    for (size_t ix = 0; ix < value->object.size; ++ix) {
        JSONNVPair const *pair = da_element_JSONNVPair(&value->object, ix);
        if (sv_eq(pair->name, attr)) {
            RETURN_VALUE(JSONValue, pair->value);
        }
    }
    RETURN_EMPTY(JSONValue);
}

JSONValue *json_get_ref(JSONValue *value, StringView attr)
{
    assert(value->type == JSON_TYPE_OBJECT);
    for (size_t ix = 0; ix < value->object.size; ++ix) {
        JSONNVPair const *pair = da_element_JSONNVPair(&value->object, ix);
        if (sv_eq(pair->name, attr)) {
            return (JSONValue *) &pair->value;
        }
    }
    return NULL;
}

JSONValue json_get_default(JSONValue *value, char const *attr, JSONValue default_)
{
    OptionalJSONValue ret_maybe = json_get(value, attr);
    if (ret_maybe.has_value) {
        return ret_maybe.value;
    }
    return default_;
}

bool json_get_bool(JSONValue *value, char const *attr, bool default_)
{
    JSONValue v = json_get_default(value, attr, json_bool(default_));
    assert(v.type == JSON_TYPE_BOOLEAN);
    return v.boolean;
}

int json_get_int(JSONValue *value, char const *attr, int default_)
{
    JSONValue v = json_get_default(value, attr, json_int(default_));
    assert(v.type == JSON_TYPE_INT);
    Integer as_i32 = MUST_OPTIONAL(Integer, integer_coerce_to(v.int_number, I32));
    return as_i32.i32;
}

StringView json_get_string(JSONValue *value, char const *attr, StringView default_)
{
    JSONValue v = json_get_default(value, attr, json_string(default_));
    assert(v.type == JSON_TYPE_STRING);
    return v.string;
}

void json_merge(JSONValue *value, JSONValue sub)
{
    assert(value->type == JSON_TYPE_OBJECT);
    assert(sub.type == JSON_TYPE_OBJECT);
    for (size_t ix = 0; ix < sub.object.size; ++ix) {
        JSONNVPair const *pair = da_element_JSONNVPair(&value->object, ix);
        json_set_sv(value, pair->name, json_copy(pair->value));
    }
}

StringView json_encode(JSONValue value)
{
    JSONEncoder encoder = { 0 };
    json_encode_to_builder(&value, &encoder);
    sv_free(encoder.escaped.view);
    return encoder.sb.view;
}

typedef struct {
    StringScanner ss;
    StringBuilder sb;
} JSONDecoder;

ErrorOrStringView json_decode_string(JSONDecoder *decoder)
{
    assert(ss_peek(&decoder->ss) == '\"');
    size_t offset = decoder->sb.view.length;
    ss_skip_one(&decoder->ss);
    ss_reset(&decoder->ss);
    while (true) {
        int ch = ss_peek(&decoder->ss);
        if (ch == '\\') {
            ss_skip_one(&decoder->ss);
            ch = ss_peek(&decoder->ss);
            switch (ch) {
            case 0:
                ERROR(StringView, JSONError, decoder->ss.point.line, "Bad escape");
            case 'n':
                sb_append_char(&decoder->sb, '\n');
                break;
            case 'r':
                sb_append_char(&decoder->sb, '\r');
                break;
            case 't':
                sb_append_char(&decoder->sb, '\t');
                break;
            case '\"':
                sb_append_char(&decoder->sb, '\"');
                break;
            default:
                sb_append_char(&decoder->sb, ch);
                break;
            }
            ss_skip_one(&decoder->ss);
        } else if (ch == '"') {
            ss_skip_one(&decoder->ss);
            StringView ret = (StringView) { .ptr = decoder->sb.view.ptr + offset, .length = decoder->sb.view.length - offset };
            RETURN(StringView, ret);
        } else if (ch == 0) {
            ERROR(StringView, JSONError, 0, "Unterminated string");
        } else {
            sb_append_char(&decoder->sb, ch);
            ss_skip_one(&decoder->ss);
        }
    }
}

ErrorOrJSONValue json_decode_value(JSONDecoder *decoder)
{
    size_t offset = decoder->sb.view.length;
    ss_skip_whitespace(&decoder->ss);
    switch (ss_peek(&decoder->ss)) {
    case 0:
        ERROR(JSONValue, JSONError, 0, "Expected value");
    case '{': {
        JSONValue result = json_object();
        ss_skip_one(&decoder->ss);
        ss_skip_whitespace(&decoder->ss);
        while (ss_peek(&decoder->ss) != '}') {
            if (ss_peek(&decoder->ss) != '"') {
                ERROR(JSONValue, JSONError, 0, "At position %zu: Expected '\"', got '%c'", decoder->ss.point, ss_peek(&decoder->ss));
            }
            StringView name = TRY_TO(StringView, JSONValue, json_decode_string(decoder));
            name = sv_copy(name);
            trace(CAT_JSON, "Name: %.*s", SV_ARG(name));
            ss_skip_whitespace(&decoder->ss);
            if (!ss_expect(&decoder->ss, ':')) {
                ERROR(JSONValue, JSONError, 0, "Expected ':'");
            }
            ss_skip_whitespace(&decoder->ss);
            JSONValue value = TRY(JSONValue, json_decode_value(decoder));
            trace(CAT_JSON, "NVP: %.*s: %.*s", SV_ARG(name), SV_ARG(json_encode(value)));
            json_set_sv(&result, name, value);
            sv_free(name);
            ss_skip_whitespace(&decoder->ss);
            ss_expect(&decoder->ss, ',');
            ss_skip_whitespace(&decoder->ss);
        }
        ss_skip_one(&decoder->ss);
        RETURN(JSONValue, result);
    }
    case '[': {
        JSONValue result = json_array();
        ss_skip_one(&decoder->ss);
        ss_skip_whitespace(&decoder->ss);
        while (ss_peek(&decoder->ss) != ']') {
            JSONValue value = TRY(JSONValue, json_decode_value(decoder));
            trace(CAT_JSON, "Array elem: %.*s", SV_ARG(json_encode(value)));
            ss_skip_whitespace(&decoder->ss);
            json_append(&result, value);
            if (ss_peek(&decoder->ss) == ',') {
                ss_skip_one(&decoder->ss);
            }
            ss_skip_whitespace(&decoder->ss);
        }
        ss_skip_one(&decoder->ss);
        RETURN(JSONValue, result);
    }
    case '"': {
        StringView str = TRY_TO(StringView, JSONValue, json_decode_string(decoder));
        RETURN(JSONValue, json_string(str));
    }
    default: {
        if (isdigit(ss_peek(&decoder->ss)) || ss_peek(&decoder->ss) == '-') {
            ss_reset(&decoder->ss);
            ss_skip_one(&decoder->ss);
            while (isdigit(ss_peek(&decoder->ss))) {
                ss_skip_one(&decoder->ss);
            }
            if (ss_peek(&decoder->ss) == '.') {
                ss_skip_one(&decoder->ss);
                while (isdigit(ss_peek(&decoder->ss))) {
                    ss_skip_one(&decoder->ss);
                }
                StringView sv = ss_read_from_mark(&decoder->ss);
                char       ch = sv.ptr[sv.length];
                ((char *) sv.ptr)[sv.length] = '\0';
                double dbl = strtod(sv.ptr, NULL);
                ((char *) sv.ptr)[sv.length] = ch;
                RETURN(JSONValue, json_number(dbl));
            }
            StringView         sv = ss_read_from_mark(&decoder->ss);
            IntegerParseResult parse_result = sv_parse_integer(sv, I64);
            if (parse_result.success) {
                RETURN(JSONValue, json_integer(parse_result.integer));
            }
            parse_result = sv_parse_integer(sv, I64);
            if (!parse_result.success) {
                ERROR(JSONValue, JSONError, 0, "Unparseable integer %.*s", SV_ARG(sv));
            }
            RETURN(JSONValue, json_integer(parse_result.integer));
        }
        if (ss_expect_sv(&decoder->ss, sv_from("true"))) {
            RETURN(JSONValue, json_bool(true));
        }
        if (ss_expect_sv(&decoder->ss, sv_from("false"))) {
            RETURN(JSONValue, json_bool(false));
        }
        if (ss_expect_sv(&decoder->ss, sv_from("null"))) {
            RETURN(JSONValue, json_null());
        }
        ERROR(JSONValue, JSONError, 0, "%d:%d: Invalid JSON", decoder->ss.point.line, decoder->ss.point.column);
    }
    }
}

ErrorOrJSONValue json_decode(StringView json)
{
    JSONDecoder      decoder = { .sb = sb_create(), .ss = ss_create(json) };
    ErrorOrJSONValue ret = json_decode_value(&decoder);
    sv_free(decoder.sb.view);
    return ret;
}

#ifdef JSON_FORMAT

#include <io.h>

int main(int argc, char **argv)
{
    if (argc != 2) {
        return -1;
    }
    log_init();
    StringView sv = MUST(StringView, read_file_by_name(sv_from(argv[1])));
    JSONValue  json = MUST(JSONValue, json_decode(sv));
    printf("%.*s\n", SV_ARG(json_encode(json)));
    return 0;
}

#endif

#ifdef JSON_TEST

int main(int argc, char **argv)
{
    JSONValue obj = json_object();
    json_set(&obj, "foo", json_string(sv_from("bar")));
    json_set(&obj, "quux", json_string(sv_from("hello")));
    StringView json = json_encode(obj);
    printf("%.*s\n", SV_ARG(json));

    ErrorOrJSONValue decoded = json_decode(json);
    if (ErrorOrJSONValue_is_error(decoded)) {
        printf("Decoding error: %s\n", Error_to_string(decoded.error));
    } else {
        json = json_encode(decoded.value);
        printf("%.*s\n", SV_ARG(json));
    }
    return 0;
}

#endif
