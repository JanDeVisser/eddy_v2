/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <log.h>
#include <mem.h>

#define SENTINEL 0xBABECAFE

void *malloc_fatal(size_t size, char const *where, ...)
{
    void *ret = malloc(size);
    if (!ret) {
        if (where) {
            va_list args;
            char    strbuf[256];
            strbuf[0] = 0;
            va_start(args, where);
            vsnprintf(strbuf, 255, where, args);
            strbuf[255] = 0;
            vfatal("Out of memory: %s", strbuf);
            va_end(args);
        }
        fatal("Out of memory");
    }
    memset(ret, 0, size);
    return ret;
}

Allocator allocator_new()
{
    Allocator ret = { 0 };
    ret.sentinel = SENTINEL;
    return ret;
}

void *allocator_allocate(Allocator alloc, size_t size)
{
    return malloc_fatal(size, NULL);
}

void *allocator_allocate_array(Allocator alloc, size_t size, size_t num)
{
    return allocator_allocate(alloc, size * num);
}

static Allocator s_alloc = { 0 };

void *mem_allocate(size_t size)
{
    if (!s_alloc.sentinel) {
        s_alloc = allocator_new();
    }
    return allocator_allocate(s_alloc, size);
}

void *mem_allocate_array(size_t count, size_t element_size)
{
    return mem_allocate(count * element_size);
}
