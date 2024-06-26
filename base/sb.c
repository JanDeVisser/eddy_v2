/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "mem.h"
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>

#include <sv.h>

extern char  *allocate_for_length(size_t length, size_t *capacity);
extern size_t buffer_capacity(char const *buffer);
extern void   buffer_free(char *buffer, size_t length);

#define SENTINEL 0xBABECAFE

char *allocate_for_length(size_t length, size_t *capacity)
{
    char  *ret = NULL;
    size_t cap = (!capacity || !*capacity) ? 32 : *capacity;
    while (cap < length + 1) {
        cap *= 2;
    }
    size_t buffer_sz = cap + 2 * sizeof(size_t);
    ret = MALLOC_ARR(char, buffer_sz);
    size_t *blockptr = (size_t *) ret;
    ret += 2 * sizeof(size_t);
    blockptr[1] = SENTINEL;
    blockptr[0] = cap | (length << 32);
    trace(SV, "SALOC:0x%08llx:%5zu", (uint64_t) ret, cap);
    if (capacity) {
        *capacity = cap;
    }
    return ret;
}

size_t buffer_capacity(char const *buffer)
{
    if (!buffer) {
        return 0;
    }
    size_t *blockptr = ((size_t *) buffer) - 2;
    if (blockptr[1] != SENTINEL) {
        return 0;
    }
    size_t ret = blockptr[0] & 0xFFFFFFFF;
    size_t length = blockptr[0] >> 32;
    assert(ret >= length);
    return ret;
}

void buffer_free(char *buffer, size_t length)
{
    size_t capacity = buffer_capacity(buffer);
    if (!capacity) {
        return;
    }
    size_t *blockptr = ((size_t *) buffer) - 2;
    size_t  block_cap = blockptr[0] & 0xFFFFFFFF;
    size_t  block_length = blockptr[0] >> 32;
    if (block_length == length) {
        free(blockptr);
    }
}

static void sb_reallocate(StringBuilder *sb, size_t new_len)
{
    char  *ret = NULL;
    size_t cap = buffer_capacity(sb->view.ptr);
    size_t new_cap = (cap) ? cap : 32;
    while (new_cap < new_len + 1)
        new_cap *= 2;
    if (new_cap <= cap) {
        return;
    }
    size_t allocated_cap = 0;
    ret = allocate_for_length(new_len, &allocated_cap);
    if (sb->view.ptr) {
        memcpy(ret, sb->view.ptr, sb->view.length);
        buffer_free((char *) sb->view.ptr, sb->view.length);
    }
    sb->view.ptr = ret;
}

StringBuilder sb_create()
{
    StringBuilder sb = { 0 };
    return sb;
}

void sb_clear(StringBuilder *sb)
{
    sb->view.length = 0;
}

StringBuilder sb_createf(char const *fmt, ...)
{
    StringBuilder ret = sb_create();
    va_list       args;
    va_start(args, fmt);
    sb_vprintf(&ret, fmt, args);
    va_end(args);
    return ret;
}

StringBuilder sb_vcreatef(char const *fmt, va_list args)
{
    StringBuilder ret = sb_create();
    sb_vprintf(&ret, fmt, args);
    return ret;
}

StringBuilder sb_copy_chars(char const *ptr, size_t len)
{
    if (ptr == NULL || len == 0) {
        return (StringBuilder) { 0 };
    }
    StringBuilder sb = sb_create();
    size_t        cap = buffer_capacity(sb.view.ptr);
    sb.view.ptr = allocate_for_length(len, &cap);
    memcpy((char *) sb.view.ptr, ptr, len);
    sb.view.length = len;
    ((char *) sb.ptr)[len] = 0;
    trace(SV, "SBCPC:0x%08llx:%5zu:%.60s", (uint64_t) sb.view.ptr, buffer_capacity(sb.view.ptr), sb.view.ptr);
    return sb;
}

StringBuilder sb_copy_sv(StringView sv)
{
    if (sv.length == 0) {
        return (StringBuilder) { 0 };
    }
    return sb_copy_chars(sv.ptr, sv.length);
}

StringBuilder sb_copy_cstr(char const *s)
{
    if (s == NULL) {
        return (StringBuilder) { 0 };
    }
    return sb_copy_chars(s, strlen(s));
}

StringRef sb_append_chars(StringBuilder *sb, char const *ptr, size_t len)
{
    if (ptr == NULL || len == 0) {
        return (StringRef) { 0 };
    }
    sb_reallocate(sb, sb->view.length + len);
    char *p = (char *) sb->view.ptr;
    memcpy(p + sb->view.length, ptr, len);
    size_t index = sb->view.length;
    sb->view.length += len;
    p[sb->view.length] = '\0';
    trace(SV, "SBAPC:0x%08llx:%5zu:%.60s", (uint64_t) sb->view.ptr, buffer_capacity(sb->view.ptr), sb->view.ptr);
    return (StringRef) { index, len };
}

StringRef sb_append_sv(StringBuilder *sb, StringView sv)
{
    return sb_append_chars(sb, sv.ptr, sv.length);
}

StringRef sb_append_cstr(StringBuilder *sb, char const *s)
{
    if (s == NULL) {
        return (StringRef) { 0 };
    }
    return sb_append_chars(sb, s, strlen(s));
}

StringRef sb_append_char(StringBuilder *sb, char ch)
{
    return sb_append_chars(sb, &ch, 1);
}

StringRef sb_append_integer(StringBuilder *sb, Integer integer)
{
    StringView ret;
    switch (integer.type) {
    case U8:
        return sb_printf(sb, "%u", integer.u8);
    case U16:
        return sb_printf(sb, "%u", integer.u16);
    case U32:
        return sb_printf(sb, "%u", integer.u32);
    case U64:
        return sb_printf(sb, "%llu", integer.u64);
    case I8:
        return sb_printf(sb, "%d", integer.i8);
    case I16:
        return sb_printf(sb, "%d", integer.i16);
    case I32:
        return sb_printf(sb, "%d", integer.i32);
    case I64:
        return sb_printf(sb, "%lld", integer.i64);
    default:
        UNREACHABLE();
    }
}

StringRef sb_append_hex_integer(StringBuilder *sb, Integer integer)
{
    switch (integer.type) {
    case U8:
        return sb_printf(sb, "%1x", integer.u8);
    case U16:
        return sb_printf(sb, "%02x", integer.u16);
    case U32:
        return sb_printf(sb, "%04x", integer.u32);
    case U64:
        return sb_printf(sb, "%08llx", integer.u64);
    default:
        UNREACHABLE();
    }
}

StringRef sb_vprintf(StringBuilder *sb, char const *fmt, va_list args)
{
    va_list args2;
    va_copy(args2, args);
    size_t len = vsnprintf(NULL, 0, fmt, args2);
    va_end(args2);
    sb_reallocate(sb, sb->view.length + len);
    size_t index = sb->view.length;
    vsnprintf((char *) sb->view.ptr + sb->view.length, len + 1, fmt, args);
    sb->view.length += len;
    trace(SV, "SBVPF:0x%08llx:%5zu:%.60s", (uint64_t) sb->view.ptr, buffer_capacity(sb->view.ptr), sb->view.ptr);
    return (StringRef) { index, len };
}

StringRef sb_printf(StringBuilder *sb, char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    StringRef ret = sb_vprintf(sb, fmt, args);
    va_end(args);
    return ret;
}

StringRef sb_insert_sv(StringBuilder *sb, StringView sv, size_t at)
{
    if (at >= sb->view.length) {
        return sb_append_sv(sb, sv);
    }
    return sb_insert_chars(sb, sv.ptr, sv.length, at);
}

StringRef sb_insert_chars(StringBuilder *sb, char const *ptr, size_t len, size_t at)
{
    if (at >= sb->view.length) {
        return sb_append_chars(sb, ptr, len);
    }
    sb_reallocate(sb, sb->view.length + len);
    char *p = (char *) sb->view.ptr;
    memmove(p + at + len, p + at, sb->view.length - at);
    memcpy(p + at, ptr, len);
    sb->view.length += len;
    p[sb->view.length] = '\0';
    trace(SV, "SBAPC:0x%08llx:%5zu:%.60s", (uint64_t) sb->view.ptr, buffer_capacity(sb->view.ptr), sb->view.ptr);
    return (StringRef) { at, len };
}

StringRef sb_insert_cstr(StringBuilder *sb, char const *str, size_t at)
{
    if (at >= sb->view.length) {
        return sb_append_cstr(sb, str);
    }
    return sb_insert_chars(sb, str, strlen(str), at);
}

void sb_remove(StringBuilder *sb, size_t at, size_t num)
{
    if (at >= sb->view.length) {
        return;
    }
    if (at + num >= sb->view.length) {
        num = sb->view.length - at;
    }
    if (num == 0) {
        return;
    }
    char *p = (char *) sb->view.ptr;
    memmove(p + at, p + at + num, sb->view.length - at);
    sb->view.length -= num;
    p[sb->view.length] = 0;
}

StringRef sb_append_list(StringBuilder *sb, StringList *sl, StringView sep)
{
    size_t index = sb->view.length;
    for (size_t ix = 0; ix < sl->size; ++ix) {
        if (ix > 0) {
            sb_append_sv(sb, sep);
        }
        sb_append_sv(sb, sl->strings[ix]);
    }
    return (StringRef) { index, sb->view.length - index };
}

int sb_replace_one(StringBuilder *sb, StringView pat, StringView repl)
{
    int loc = sv_find(sb->view, pat);
    if (loc != -1) {
        sb_remove(sb, loc, pat.length);
        sb_insert_sv(sb, repl, loc);
    }
    return loc;
}

int sb_replace_all(StringBuilder *sb, StringView pat, StringView repl)
{
    int ret = 0;
    for (int loc = sv_find(sb->view, pat); loc != -1; loc = sv_find_from(sb->view, pat, loc + repl.length)) {
        sb_remove(sb, loc, pat.length);
        sb_insert_sv(sb, repl, loc);
        ++ret;
    }
    return ret;
}

StringView sb_view(StringBuilder *sb)
{
    return sb->view;
}

StringView sv(StringBuilder *sb, StringRef ref)
{
    if (ref.length == 0) {
        return sv_null();
    }
    return (StringView) { sb->view.ptr + ref.index, ref.length };
}
