/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BASE_FMT_H
#define BASE_FMT_H

#include <stdint.h>

#include <base/da.h>
#include <base/sv.h>

typedef enum {
    FMT_INTEGER,
    FMT_FLOAT,
    FMT_STRING,
    FMT_POINTER,
    FMT_CSTR,
} ArgType;

typedef struct fmt_arg {
    ArgType type;
    union {
        Integer     integer;
        double      flt;
        StringView  sv;
        void       *pointer;
        char const *cstr;
    };
} FMTArg;

DA(FMTArg)
typedef DA_FMTArg FMTArgs;

extern StringView fmt_format(StringView fmt, FMTArgs);
extern StringView vformat(StringView fmt, va_list args);
extern StringView format(StringView fmt, ...);

#endif /* BASE_FMT_H */
