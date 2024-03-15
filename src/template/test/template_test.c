/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <io.h>
#include <template.h>

#define RED "\033[31;1m"
#define GREEN "\033[32;1m"
#define YELLOW "\033[33;1m"
#define RESET "\033[0m"

StringView value_to_string(JSONValue value)
{
    switch (value.type) {
    case JSON_TYPE_STRING: {
        return value.string;
    } break;
    case JSON_TYPE_ARRAY: {
        StringBuilder sb = { 0 };
        for (size_t ix = 0; ix < json_len(&value); ++ix) {
            JSONValue entry = MUST_OPTIONAL(JSONValue, json_at(&value, ix));
            if (entry.type == JSON_TYPE_STRING) {
                if (sb.length > 0) {
                    sb_append_char(&sb, '\n');
                }
                sb_append_sv(&sb, entry.string);
            } else {
                printf("Invalid type in array\n");
                exit(1);
            }
        }
        return sb.view;
    };
    default: {
        printf("Invalid type\n");
        exit(1);
    }
    }
}

int test(StringView name)
{
    ErrorOrStringView json_maybe = read_file_by_name(sv_printf("%.*s.json", SV_ARG(name)));
    if (ErrorOrStringView_is_error(json_maybe)) {
        printf(SV_SPEC_LALIGN ": " RED "FAIL" RESET "\n", SV_ARG_LALIGN(name, 20));
        printf("%s\n", Error_to_string(json_maybe.error));
        return 1;
    }
    StringView json = json_maybe.value;

    ErrorOrJSONValue test_maybe = json_decode(json);
    if (ErrorOrStringView_is_error(json_maybe)) {
        printf(SV_SPEC_LALIGN ": " RED "FAIL" RESET "\n", SV_ARG_LALIGN(name, 20));
        printf("%s\n", Error_to_string(test_maybe.error));
        return 1;
    }
    JSONValue test = test_maybe.value;

    OptionalJSONValue skipped_maybe = json_get(&test, "skip");
    if (skipped_maybe.has_value) {
        assert(skipped_maybe.value.type == JSON_TYPE_BOOLEAN);
        if (skipped_maybe.value.boolean) {
            printf(SV_SPEC_LALIGN ": " YELLOW "SKIP" RESET "\n", SV_ARG_LALIGN(name, 20));
            return 0;
        }
    }
    JSONValue         context = json_get_default(&test, "ctx", json_object());
    OptionalJSONValue template_json_maybe = json_get(&test, "template");
    if (!template_json_maybe.has_value) {
        printf(SV_SPEC_LALIGN ": " RED "FAIL" RESET "\n", SV_ARG_LALIGN(name, 20));
        printf("No template found\n");
        return 1;
    }
    StringView template = value_to_string(template_json_maybe.value);
    ErrorOrStringView rendered_maybe = render_template(template, context);
    if (ErrorOrStringView_is_error(rendered_maybe)) {
        printf(SV_SPEC_LALIGN ": " RED "FAIL" RESET "\n", SV_ARG_LALIGN(name, 20));
        printf("%s\n", Error_to_string(rendered_maybe.error));
        return 1;
    }
    StringView rendered = rendered_maybe.value;
    JSONValue  expected_json = json_get_default(&test, "expected", json_string(sv_null()));
    StringView expected = value_to_string(expected_json);
    if (sv_eq(rendered, expected)) {
        printf(SV_SPEC_LALIGN ": " GREEN "OK" RESET "\n", SV_ARG_LALIGN(name, 20));
    } else {
        printf(SV_SPEC_LALIGN ": " RED "FAIL" RESET "\n", SV_ARG_LALIGN(name, 20));
        printf("Expected: %.*s\n", SV_ARG(expected));
        printf("Rendered: %.*s\n", SV_ARG(rendered));
        return 1;
    }
    return 0;
}

int run_all_tests()
{
    StringView        tests_name = sv_from("tests.json");
    ErrorOrStringView json_maybe = read_file_by_name(sv_from("tests.json"));
    if (ErrorOrStringView_is_error(json_maybe)) {
        printf(SV_SPEC_LALIGN ": " RED "FAIL" RESET "\n", SV_ARG_LALIGN(tests_name, 20));
        printf("%s\n", Error_to_string(json_maybe.error));
        return 1;
    }
    StringView json = json_maybe.value;

    ErrorOrJSONValue tests_maybe = json_decode(json);
    if (ErrorOrStringView_is_error(json_maybe)) {
        printf(SV_SPEC_LALIGN ": " RED "FAIL" RESET "\n", SV_ARG_LALIGN(tests_name, 20));
        printf("%s\n", Error_to_string(tests_maybe.error));
        return 1;
    }

    JSONValue tests = tests_maybe.value;
    if (tests.type != JSON_TYPE_ARRAY) {
        printf(SV_SPEC_LALIGN ": " RED "FAIL" RESET "\n", SV_ARG_LALIGN(tests_name, 20));
        printf("%.*s should contain a single array\n", SV_ARG(tests_name));
        return 1;
    }

    int ret = 0;
    for (size_t ix = 0; ix < json_len(&tests); ++ix) {
        JSONValue entry = MUST_OPTIONAL(JSONValue, json_at(&tests, ix));
        if (entry.type != JSON_TYPE_STRING) {
            StringView s = json_to_string(entry);
            printf(SV_SPEC_LALIGN ": " RED "FAIL" RESET "\n", SV_ARG_LALIGN(s, 20));
            printf("Tests must be refered to by their name as strings\n");
            ++ret;
            continue;
        }

        StringView name = entry.string;
        int        result = test(name);
        if (result != 0) {
            ++ret;
        }
    }
    return ret;
}

int main(int argc, char **argv)
{
    log_init();
    if (argc == 1) {
        return true - (run_all_tests() == 0);
    }
    int ret = 0;
    for (int ix = 1; ix < argc; ++ix) {
        StringView name = sv_from(argv[ix]);
        if (test(name) != 0) {
            ++ret;
        }
    }
    return true - (ret == 0);
}
