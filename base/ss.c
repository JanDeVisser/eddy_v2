/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <ctype.h>

#include <sv.h>

StringScanner ss_create(StringView sv)
{
    StringScanner ret = { 0 };
    ret.string = sv;
    return ret;
}

void ss_rewind(StringScanner *ss)
{
    ss->point = ss->mark;
}

void ss_reset(StringScanner *ss)
{
    ss->mark = ss->point;
}

void ss_partial_rewind(StringScanner *ss, size_t num)
{
    if (num > (ss->point.index - ss->mark.index)) {
        num = ss->point.index - ss->mark.index;
    }
    if (num > 0) {
        ss_rewind(ss);
        ss_skip(ss, ss->point.index - ss->mark.index - num);
    }
}

void ss_pushback(StringScanner *ss)
{
    ss_partial_rewind(ss, 1);
}

StringView ss_read(StringScanner *ss, size_t num)
{
    if ((int64_t) (num) < 0) {
        num = 0;
    }
    if ((ss->point.index + num) > ss->string.length) {
        num = ss->string.length - ss->point.index;
    }
    StringView ret = sv_substring(ss->string, ss->point.index, num);
    ss_skip(ss, num);
    return ret;
}

StringView ss_read_from_mark(StringScanner *ss)
{
    size_t num = ss->point.index - ss->mark.index;
    if (num > 0) {
        return sv_substring(ss->string, ss->mark.index, ss->point.index - ss->mark.index);
    }
    return sv_null();
}

int ss_readchar(StringScanner *ss)
{
    ss_skip_one(ss);
    return (ss->point.index <= ss->string.length - 1) ? ss->string.ptr[ss->point.index] : '\0';
}

int ss_peek(StringScanner *ss)
{
    return ss_peek_with_offset(ss, 0);
}

int ss_peek_with_offset(StringScanner *ss, size_t offset)
{
    return ((ss->point.index + offset) < ss->string.length) ? ss->string.ptr[ss->point.index + offset] : 0;
}

StringView ss_peek_sv(StringScanner *ss, size_t length)
{
    if (ss->point.index + length > ss->string.length) {
        length = ss->string.length - ss->point.index;
    }
    if (length == 0) {
        return sv_null();
    }
    return (StringView) { .ptr = ss->string.ptr + ss->point.index, length };
}

StringView ss_peek_tail(StringScanner *ss)
{
    return sv_lchop(ss->string, ss->point.index);
}

void ss_skip(StringScanner *ss, size_t num)
{
    if (ss->point.index + num > ss->string.length) {
        num = ss->string.length - ss->point.index;
    }
    for (size_t new_index = ss->point.index + num; ss->point.index < new_index; ++ss->point.index) {
        if (ss->string.ptr[ss->point.index] == '\n') {
            ++ss->point.line;
            ss->point.column = 0;
        } else {
            ++ss->point.column;
        }
    }
}

void ss_skip_one(StringScanner *ss)
{
    ss_skip(ss, 1);
}

void ss_skip_whitespace(StringScanner *ss)
{
    while (isspace(ss_peek(ss))) {
        ss_skip(ss, 1);
    }
}

void ss_skip_until(StringScanner *ss, int ch)
{
    while (ss_peek(ss) && ss_peek(ss) != ch) {
        ss_skip_one(ss);
    }
}

bool ss_expect_with_offset(StringScanner *ss, char ch, size_t offset)
{
    if (ss_peek_with_offset(ss, offset) != ch) {
        return false;
    }
    ss_skip(ss, offset + 1);
    return true;
}

bool ss_expect(StringScanner *ss, char ch)
{
    return ss_expect_with_offset(ss, ch, 0);
}

bool ss_expect_sv(StringScanner *ss, StringView sv)
{
    if (ss->point.index + sv.length > ss->string.length) {
        return false;
    }
    StringView s = sv_substring(ss->string, ss->point.index, sv.length);
    if (!sv_eq(s, sv)) {
        return false;
    }
    ss_skip(ss, sv.length);
    return true;
}

bool ss_is_one_of_with_offset(StringScanner *ss, char const *expect, size_t offset)
{
    return strchr(expect, ss_peek_with_offset(ss, offset)) != NULL;
}

bool ss_is_one_of(StringScanner *ss, char const *expect)
{
    return ss_is_one_of_with_offset(ss, expect, 0);
}

bool ss_expect_one_of_with_offset(StringScanner *ss, char const *expect, size_t offset)
{
    if (ss_is_one_of_with_offset(ss, expect, offset)) {
        ss_skip(ss, offset + 1);
        return true;
    }
    return false;
}

bool ss_expect_one_of(StringScanner *ss, char const *expect)
{
    return ss_expect_one_of_with_offset(ss, expect, 0);
}

int ss_one_of(StringScanner *ss, char const *expect)
{
    if (strchr(expect, ss_peek(ss)) != NULL) {
        return ss_readchar(ss);
    }
    return 0;
}

size_t ss_read_number(StringScanner *ss)
{
    size_t ix = 0;
    while (isdigit(ss_peek_with_offset(ss, ix))) {
        ix++;
    }
    if (ix > 0) {
        StringView num = ss_read(ss, ix);
        ss_reset(ss);
        IntegerParseResult parse_result = sv_parse_u64(num);
        assert(parse_result.success);
        return parse_result.integer.u64;
    }
    return 0;
};
