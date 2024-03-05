/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <error_or.h>
#include <mem.h>

char const *ErrorCategory_name(ErrorCategory cat)
{
    switch (cat) {
#undef ERRORCATEGORY_ENUM
#define ERRORCATEGORY_ENUM(code, value) \
    case code:                          \
        return #code;
        ERRORCATEGORIES(ERRORCATEGORY_ENUM)
#undef ERRORCATEGORY_ENUM
    default:
        UNREACHABLE();
    }
}

char const *Error_to_string(Error error)
{
#undef _ERROR_MSG_FORMAT
#define _ERROR_MSG_FORMAT "ERROR: %s:%d %s(%d): %s"
    size_t str_len = snprintf(
                         NULL, 0, _ERROR_MSG_FORMAT,
                         error.file, error.line, ErrorCategory_name(error.cat), error.code, error.message)
        + 1;
    char *str = mem_allocate(str_len);
    snprintf(str, str_len, _ERROR_MSG_FORMAT,
        error.file, error.line,
        ErrorCategory_name(error.cat), error.code, error.message);
#undef _ERROR_MSG_FORMAT
    return str;
}
