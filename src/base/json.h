/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __JSON_H__
#define __JSON_H__

#include <integer.h>
#include <sv.h>

#define JSONTYPES(S) \
    S(NULL)          \
    S(OBJECT)        \
    S(ARRAY)         \
    S(STRING)        \
    S(INT)           \
    S(DOUBLE)        \
    S(BOOLEAN)

typedef enum {
#undef S
#define S(T) JSON_TYPE_##T,
    JSONTYPES(S)
#undef S
} JSONType;

DA_VOID_WITH_NAME(JSONValue, JSONValues);
DA_VOID_WITH_NAME(JSONNVPair, JSONNVPairs);

typedef struct json_value {
    JSONType type;
    union {
        JSONNVPairs object;
        JSONValues  array;
        StringView  string;
        Integer     int_number;
        double      double_number;
        bool        boolean;
    };
} JSONValue;

typedef struct {
    StringView name;
    JSONValue  value;
} JSONNVPair;

DA_FUNCTIONS(JSONValue)
DA_FUNCTIONS(JSONNVPair)
OPTIONAL(JSONValue)
ERROR_OR(JSONValue)
OPTIONAL(JSONValues)

extern char const       *JSONType_name(JSONType type);
extern JSONValue         json_object(void);
extern JSONValue         json_array(void);
extern JSONValue         json_null(void);
extern JSONValue         json_string(StringView sv);
extern JSONValue         json_number(double number);
extern JSONValue         json_int(int number);
extern JSONValue         json_integer(Integer number);
extern int               json_int_value(JSONValue value);
extern JSONValue         json_bool(bool value);
extern void              json_free(JSONValue value);
extern JSONValue         json_move(JSONValue *from);
extern JSONValue         json_copy(JSONValue from);
extern int               json_compare(JSONValue a, JSONValue b);
extern void              json_append(JSONValue *array, JSONValue elem);
extern void              json_add(JSONValue *array, JSONValue elem);
extern void              json_concat(JSONValue *array, JSONValue other);
extern OptionalInt       json_find(JSONValue *array, JSONValue elem);
extern OptionalJSONValue json_at(JSONValue *array, size_t index);
extern JSONValue        *json_at_ref(JSONValue *array, size_t index);
extern size_t            json_len(JSONValue *array);
extern void              json_set(JSONValue *value, char const *attr, JSONValue elem);
extern void              json_optional_set(JSONValue *value, char const *attr, OptionalJSONValue elem);
extern void              json_set_string(JSONValue *value, char const *attr, StringView sv);
extern void              json_set_cstr(JSONValue *value, char const *attr, char const *s);
extern void              json_set_int(JSONValue *value, char const *attr, int i);
extern void              json_set_int_sv(JSONValue *value, StringView attr, int i);
extern void              json_set_sv(JSONValue *value, StringView attr, JSONValue elem);
extern OptionalJSONValue json_entry_at(JSONValue *value, int ix);
extern OptionalJSONValue _json_get(JSONValue *value, ...);
// extern OptionalJSONValue json_get(JSONValue *value, char const *attr);
extern OptionalJSONValue json_get_sv(JSONValue *value, StringView attr);
extern JSONValue        *json_get_ref(JSONValue *value, StringView attr);
extern JSONValue         json_get_default(JSONValue *value, char const *attr, JSONValue default_);
extern bool              json_get_bool(JSONValue *value, char const *attr, bool default_);
extern int               json_get_int(JSONValue *value, char const *attr, int default_);
extern StringView        json_get_string(JSONValue *value, char const *attr, StringView default_);
extern bool              json_has(JSONValue *value, char const *attr);
extern bool              json_has_sv(JSONValue *value, StringView attr);
extern void              json_delete(JSONValue *value, char const *attr);
extern void              json_delete_sv(JSONValue *value, StringView attr);
extern void              json_merge(JSONValue *value, JSONValue sub);
extern StringView        json_to_string(JSONValue value);
extern StringView        json_encode(JSONValue value);
extern ErrorOrJSONValue  json_decode(StringView json_text);

#define json_get(JSON, ...) _json_get((JSON) __VA_OPT__(, ) __VA_ARGS__, NULL)

#endif /* __JSON_H__ */
