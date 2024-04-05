/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <pthread.h>
#include <stdarg.h>
#include <unistd.h>

#include <log.h>
#include <options.h>
#include <sv.h>

typedef enum log_level {
    LL_TRACE,
    LL_INFO,
    LL_PANIC,
} LogLevel;

static StringList s_categories = {0};

static void vemit_log_message(LogLevel level, char const *file_name, int line, TraceCategory category, char const *msg, va_list args);
static void emit_log_message(LogLevel level, char const *file_name, int line, TraceCategory category, char const *msg, ...);

static LogLevel log_level = LL_INFO;

static char const *log_level_to_string(LogLevel level)
{
    switch (level) {
    case LL_TRACE:
        return "TRACE";
    case LL_INFO:
        return "INFO";
    case LL_PANIC:
        return "PANIC";
    default:
        UNREACHABLE();
    }
}

void emit_log_message(LogLevel level, char const *file_name, int line, TraceCategory category, char const *msg, ...)
{
    va_list args;
    va_start(args, msg);
    vemit_log_message(level, file_name, line, category, msg, args);
    va_end(args);
}

static size_t linelen = 0;

void vemit_log_message(LogLevel level, char const *file_name, int line, TraceCategory category, char const *msg, va_list args)
{
    if (level < log_level) {
        return;
    }
    if (!_log_category_on(category)) {
        return;
    }
    if (linelen == 0) {
        StringView cat = category;
        if (level >= LL_INFO) {
            cat = sv_from(log_level_to_string(level));
        }
        char buf[32];
        snprintf(buf, 32, "%s:%d", file_name, line);
        fprintf(stderr, "%-*.*s:[%05d:%08llx]:%c:%7.7s: ", 15, 15, buf, getpid(), (uint64_t) pthread_self(), *cat.ptr, cat.ptr);
    }
    linelen += vfprintf(stderr, msg, args);
}

void log_nl(LogLevel level, TraceCategory category)
{
    if (level < log_level) {
        return;
    }
    if (!_log_category_on(category)) {
        return;
    }
    fprintf(stderr, "\n");
    linelen = 0;
}

void _trace_nl(TraceCategory category)
{
    if (!_log_category_on(category)) {
        return;
    }
    log_nl(LL_TRACE, category);
    linelen = 0;
}

void _trace(char const *file_name, int line, TraceCategory category, char const *msg, ...)
{
    if (!_log_category_on(category)) {
        return;
    }
    va_list args;
    va_start(args, msg);
    vtrace(file_name, line, category, msg, args);
    va_end(args);
}

void vtrace(char const *file_name, int line, TraceCategory category, char const *msg, va_list args)
{
    if (!_log_category_on(category)) {
        return;
    }
    vemit_log_message(LL_TRACE, file_name, line, category, msg, args);
    _trace_nl(category);
}

void _panic(char const *file_name, int line, char const *msg, ...)
{
    va_list args;
    va_start(args, msg);
    vpanic(file_name, line, msg, args);
    va_end(args);
}

void vpanic(char const *file_name, int line, char const *msg, va_list args)
{
    vemit_log_message(LL_PANIC, file_name, line, sv_null(), msg, args);
    log_nl(LL_PANIC, sv_null());
}

void _info(char const *file_name, int line, char const *msg, ...)
{
    va_list args;
    va_start(args, msg);
    vinfo(file_name, line, msg, args);
    va_end(args);
}

void _info_nonl(char const *file_name, int line, char const *msg, ...)
{
    va_list args;
    va_start(args, msg);
    vinfo_nonl(file_name, line, msg, args);
    va_end(args);
}

void vinfo(char const *file_name, int line, char const *msg, va_list args)
{
    vinfo_nonl(file_name, line, msg, args);
    log_nl(LL_INFO, sv_null());
}

void vinfo_nonl(char const *file_name, int line, char const *msg, va_list args)
{
    vemit_log_message(LL_INFO, file_name, line, sv_null(), msg, args);
}

void _trace_nonl(char const *file_name, int line, TraceCategory category, char const *msg, ...)
{
    if (!_log_category_on(category)) {
        return;
    }
    va_list args;
    va_start(args, msg);
    vtrace_nonl(file_name, line, category, msg, args);
    va_end(args);
}

void vtrace_nonl(char const *file_name, int line, TraceCategory category, char const *msg, va_list args)
{
    if (!_log_category_on(category)) {
        return;
    }
    vemit_log_message(LL_TRACE, file_name, line, category, msg, args);
}

bool _log_category_on(TraceCategory category)
{
    if (sv_empty(category)) {
        return true;
    }
    for (size_t ix = 0; ix < s_categories.size; ++ix) {
        if (sv_eq(s_categories.strings[ix], category) || sv_eq(s_categories.strings[ix], SV("true", 4))) {
            return true;
        }
    }
    return false;
}

void log_turn_on_sv(TraceCategory category)
{
    for (size_t ix = 0; ix < s_categories.size; ++ix) {
        if (sv_eq(s_categories.strings[ix], category)) {
            return;
        }
    }
    sl_push(&s_categories, category);
}

void log_turn_off_sv(StringView category)
{
    for (size_t ix = 0; ix < s_categories.size; ++ix) {
        if (sv_eq(s_categories.strings[ix], category)) {
            return;
        }
    }
    sl_push(&s_categories, category);
}

void _fatal(char const *file_name, int line, char const *msg, ...)
{
    va_list args;
    va_start(args, msg);
    vfatal(file_name, line, msg, args);
}

void vfatal(char const *file_name, int line, char const *msg, va_list args)
{
    vpanic(file_name, line, msg, args);
    _panic(file_name, line, "Aborting...");
    exit(1);
}

void log_init()
{
    StringList categories = get_option_values(sv_from("trace"));
    if (sl_empty(&categories)) {
        char const *cats = getenv("TRACE");
        categories = sv_split(sv_from(cats), sv_from(";"));
    }
    log_level = (!sl_empty(&categories)) ? LL_TRACE : LL_PANIC;
    if (log_level == LL_PANIC) {
        info("Tracing disabled");
        return;
    }
    for (size_t ix = 0; ix < sl_size(&categories); ++ix) {
        sl_push(&s_categories, sv_copy(categories.strings[ix]));
    }
    linelen = 0;
    StringView cats = sl_join(&s_categories, SV(", ", 2));
    info("Tracing initialized. Enabled categories: %.*s", SV_ARG(cats));
}
