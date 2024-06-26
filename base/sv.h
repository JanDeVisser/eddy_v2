/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SV_H__
#define __SV_H__

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <base/da.h>
#include <base/error_or.h>
#include <base/integer.h>
#include <base/mem.h>
#include <base/optional.h>

struct string_view {
    char const *ptr;
    size_t      length;
};
typedef struct string_view StringView;

OPTIONAL(StringView)
ERROR_OR(StringView);
ERROR_OR(OptionalStringView);

DA_ELEMENTS(StringView, strings)
typedef DA_StringView StringList;
typedef DA_StringView StringViews;
ERROR_OR(StringList);
OPTIONAL(StringList);
typedef OptionalStringList OptionalStringViews;

typedef struct string_builder {
    union {
        StringView view;
        struct {
            char const *ptr;
            size_t      length;
        };
    };
} StringBuilder;

DA(StringBuilder)

typedef struct {
    size_t index;
    size_t length;
} StringRef;

OPTIONAL(StringRef);
ERROR_OR(StringRef);
ERROR_OR(OptionalStringRef);
DA_WITH_NAME(StringRef, StringRefs);

#define INTEGER_SIZES(S) S(8) S(16) S(32) S(64)

typedef struct integer_parse_result {
    bool       success;
    StringView tail;
    Integer    integer;
} IntegerParseResult;

typedef struct {
    size_t index;
    size_t line;
    size_t column;
} TextPosition;

typedef struct StringScanner {
    StringView   string;
    TextPosition mark;
    TextPosition point;
} StringScanner;

#define SV(S, ...) \
    (StringView) { .ptr = (S), .length = strlen(S) }

extern StringView         sv_null();
extern void               sv_free(StringView sv);
extern StringView         sv_copy(StringView sv);
extern StringView         sv_copy_chars(char const *ptr, size_t len);
extern StringView         sv_copy_cstr(char const *s);
extern ErrorOrStringView  sv_read(int fd, size_t num);
extern StringView         sv_render_integer(Integer integer);
extern StringView         sv_render_hex_integer(Integer integer);
extern StringView         sv_printf(char const *fmt, ...) format_args(1, 2);
extern StringView         sv_vprintf(char const *fmt, va_list args);
extern StringView         sv_replicate(StringView s, int repeats);
extern StringView         sv_from(char const *s);
extern StringView         sv_decode_quoted_str(StringView str);
extern StringView         sv_replace(StringView str, StringView from, StringView to);
extern bool               sv_empty(StringView sv);
extern bool               sv_not_empty(StringView sv);
extern bool               sv_is_whitespace(StringView sv);
extern size_t             sv_length(StringView sv);
extern unsigned int       sv_hash(StringView *sv);
extern bool               sv_is_cstr(StringView sv);
extern char const        *sv_cstr(StringView sv, char *buffer);
extern int                sv_cmp(StringView s1, StringView s2);
extern int                sv_icmp(StringView s1, StringView s2);
extern bool               sv_eq(StringView s1, StringView s2);
extern bool               sv_eq_cstr(StringView s1, char const *s2);
extern bool               sv_eq_chars(StringView s1, char const *s2, size_t n);
extern bool               sv_eq_ignore_case(StringView s1, StringView s2);
extern bool               sv_eq_ignore_case_cstr(StringView s1, char const *s2);
extern bool               sv_eq_ignore_case_chars(StringView s1, char const *s2, size_t n);
extern bool               sv_startswith(StringView s1, StringView s2);
extern bool               sv_endswith(StringView s1, StringView s2);
extern IntegerParseResult sv_parse_integer(StringView sv, IntegerType type);
extern StringView         sv_lchop(StringView sv, size_t num);
extern StringView         sv_rchop(StringView sv, size_t num);
extern StringView         sv_chop_to_delim(StringView *src, StringView delim);
extern int                sv_first(StringView sv, char ch);
extern int                sv_last(StringView sv, char ch);
extern int                sv_find(StringView sv, StringView sub);
extern int                sv_find_from(StringView sv, StringView sub, size_t from);
extern StringView         sv_substring(StringView sv, size_t at, size_t len);
extern StringList         sv_split(StringView sv, StringView sep);
extern StringList         sv_split_by_whitespace(StringView sv);
extern StringView         sv_strip(StringView sv);

#undef INTEGER_SIZE
#define INTEGER_SIZE(sz)                                     \
    extern IntegerParseResult sv_parse_u##sz(StringView sv); \
    extern IntegerParseResult sv_parse_i##sz(StringView sv);
INTEGER_SIZES(INTEGER_SIZE)
#undef INTEGER_SIZE

#define SV_SPEC "%.*s"
#define SV_SPEC_RALIGN "%*.s%.*s"
#define SV_SPEC_LALIGN "%.*s%*.s"
#define SV_ARG(sv) (int) sv.length, sv.ptr
#define SV_ARG_RALIGN(sv, width) (int) (width - sv.length), "", (int) sv.length, sv.ptr
#define SV_ARG_LALIGN(sv, width) (int) sv.length, sv.ptr, (int) (width - sv.length), ""

extern StringBuilder sb_create();
extern StringBuilder sb_createf(char const *fmt, ...) format_args(1, 2);
extern StringBuilder sb_vcreatef(char const *fmt, va_list args);
extern StringBuilder sb_copy_chars(char const *ptr, size_t len);
extern StringBuilder sb_copy_cstr(char const *s);
extern StringBuilder sb_copy_sv(StringView sv);
extern void          sb_clear(StringBuilder *sb);
extern StringRef     sb_append_chars(StringBuilder *sb, char const *ptr, size_t len);
extern StringRef     sb_append_sv(StringBuilder *sb, StringView sv);
extern StringRef     sb_append_cstr(StringBuilder *sb, char const *s);
extern StringRef     sb_append_char(StringBuilder *sb, char ch);
extern StringRef     sb_append_integer(StringBuilder *sb, Integer integer);
extern StringRef     sb_append_hex_integer(StringBuilder *sb, Integer integer);
extern StringRef     sb_vprintf(StringBuilder *sb, char const *fmt, va_list args);
extern StringRef     sb_printf(StringBuilder *sb, char const *fmt, ...) format_args(2, 3);
extern StringRef     sb_insert_sv(StringBuilder *sb, StringView sv, size_t at);
extern StringRef     sb_insert_chars(StringBuilder *sb, char const *ptr, size_t len, size_t at);
extern StringRef     sb_insert_cstr(StringBuilder *sb, char const *str, size_t at);
extern void          sb_remove(StringBuilder *sb, size_t at, size_t num);
extern StringRef     sb_append_list(StringBuilder *sb, StringList *sl, StringView sep);
extern int           sb_replace_one(StringBuilder *sb, StringView pat, StringView repl);
extern int           sb_replace_all(StringBuilder *sb, StringView pat, StringView repl);
extern StringView    sb_view(StringBuilder *sb);
extern StringView    sv(StringBuilder *sb, StringRef ref);

#define SB_SPEC SV_SPEC
#define SB_ARG(sb) (int) (sb).view.length, (sb).view.ptr

extern StringList  sl_create();
extern void        sl_free(StringList *sl);
extern StringList  sl_copy(StringList *sl);
extern StringList *sl_push(StringList *sl, StringView sv);
extern StringList *sl_extend(StringList *sl, StringList *with);
extern StringView  sl_pop(StringList *sl);
extern StringView  sl_pop_front(StringList *sl);
extern StringView  sl_join(StringList *sl, StringView sep);
extern StringList  sl_split(StringList *sl, size_t at);
extern StringView  sl_front(StringList *sl);
extern StringView  sl_back(StringList *sl);
extern bool        sl_empty(StringList *sl);
extern size_t      sl_size(StringList *sl);
extern bool        sl_has(StringList *sl, StringView sv);

extern StringScanner ss_create(StringView sv);
extern void          ss_rewind(StringScanner *ss);
extern void          ss_reset(StringScanner *ss);
extern bool          ss_expect(StringScanner *ss, char ch);
extern bool          ss_expect_sv(StringScanner *ss, StringView sv);
extern bool          ss_expect_with_offset(StringScanner *ss, char ch, size_t offset);
extern bool          ss_expect_one_of_with_offset(StringScanner *ss, char const *expect, size_t offset);
extern bool          ss_expect_one_of(StringScanner *ss, char const *expect);
extern bool          ss_is_one_of(StringScanner *ss, char const *expect);
extern bool          ss_is_one_of_with_offset(StringScanner *ss, char const *expect, size_t offset);
extern int           ss_one_of(StringScanner *ss, char const *expect);
extern StringView    ss_read(StringScanner *ss, size_t num);
extern StringView    ss_read_from_mark(StringScanner *ss);
extern int           ss_readchar(StringScanner *ss);
extern int           ss_peek(StringScanner *ss);
extern int           ss_peek_with_offset(StringScanner *ss, size_t offset);
extern StringView    ss_peek_sv(StringScanner *ss, size_t length);
extern StringView    ss_peek_tail(StringScanner *ss);
extern void          ss_skip(StringScanner *ss, size_t num);
extern void          ss_skip_one(StringScanner *ss);
extern void          ss_skip_whitespace(StringScanner *ss);
extern void          ss_skip_until(StringScanner *ss, int ch);
extern void          ss_pushback(StringScanner *ss);
extern size_t        ss_read_number(StringScanner *ss);

#endif /* __SV_H__ */
