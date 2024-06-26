/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#define HAVE_DLFCN_H

#ifndef __RESOLVE_H__
#define __RESOLVE_H__

#include <base/sv.h>

#ifdef HAVE_DLFCN_H
typedef void *lib_handle_t;
typedef int   resolve_error_t;
#elif defined(HAVE_WINDOWS_H)
typedef HMODULE lib_handle_t;
typedef DWORD   resolve_error_t;
#endif /* HAVE_DLFCN_H */

typedef void (*void_t)();

typedef struct function_handle {
    StringView              name;
    void                   *function;
    struct function_handle *next;
} FunctionHandle;

typedef struct _resolve_handle {
    lib_handle_t            handle;
    StringView              image;
    StringView              platform_image;
    FunctionHandle         *functions;
    struct _resolve_handle *next;
} LibHandle;

typedef struct _resolve {
    LibHandle *images;
} Resolve;

Resolve   *resolve_get(void);
void       resolve_free(void);
LibHandle *resolve_open(Resolve *, StringView);
void_t     resolve_resolve(Resolve *, StringView, StringView);
bool       resolve_library(StringView);
void_t     resolve_function(char const *);

#endif /* __RESOLVE_H__ */
