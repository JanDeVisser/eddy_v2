/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdarg.h>
#include <stdbool.h>
#include <unistd.h>

#include <log.h>
#include <options.h>
#include <pthread.h>
#include <sv.h>

typedef enum log_level {
    LL_TRACE,
    LL_PANIC,
} LogLevel;

static bool s_categories[CAT_COUNT];

static void vemit_log_message(LogLevel level, TraceCategory category, char const *msg, va_list args);
static void emit_log_message(LogLevel level, TraceCategory category, char const *msg, ...);

static LogLevel log_level = LL_PANIC;

static char const* trace_category_to_string(TraceCategory category)
{
    switch(category) {
#undef TRACECATEGORY
#define TRACECATEGORY(cat) case CAT_##cat: return #cat;
        TRACECATEGORIES(TRACECATEGORY)
    #undef TRACECATEGORY
        case CAT_COUNT: return "";
    default:
        UNREACHABLE();
    }
};



void emit_log_message(LogLevel level, TraceCategory category, char const *msg, ...)
{
    va_list args;
    va_start(args, msg);
    vemit_log_message(level, category, msg, args);
    va_end(args);
}

void vemit_log_message(LogLevel level, TraceCategory category, char const *msg, va_list args)
{
    if (level >= log_level) {
        fprintf(stderr, "[%05d:%08llx]:%7.7s: ", getpid(), (uint64_t) pthread_self(), trace_category_to_string(category));
        vfprintf(stderr, msg, args);
        fprintf(stderr, "\n");
    }
}

void trace(TraceCategory category, char const *msg, ...)
{
    if (!s_categories[(int) category]) {
        return;
    }
    va_list args;
    va_start(args, msg);
    vemit_log_message(LL_TRACE, category, msg, args);
    va_end(args);
}

void vtrace(char const *msg, va_list args)
{
    vemit_log_message(LL_TRACE, CAT_COUNT, msg, args);
}

void _fatal(char const *msg, ...)
{
    va_list args;
    va_start(args, msg);
    vfatal(msg, args);
}

void vfatal(char const *msg, va_list args)
{
    vemit_log_message(LL_PANIC, CAT_COUNT, msg, args);
    emit_log_message(LL_PANIC, CAT_COUNT, "Aborting...");
    exit(1);
}

void log_init()
{
    for (int c = 0; c < (int) CAT_COUNT; ++c) {
        s_categories[c] = false;
    }
    StringList categories = get_option_values(sv_from("trace"));
    if (sl_empty(&categories)) {
        char const *cats = getenv("TRACE");
        categories = sv_split(sv_from(cats), sv_from(";"));
    }
    log_level = (!sl_empty(&categories)) ? LL_TRACE : LL_PANIC;
    if (log_level == LL_PANIC) {
        return;
    }
    for (size_t ix = 0; ix < sl_size(&categories); ++ix) {
        if (sv_eq_cstr(categories.strings[ix], "true")) {
            for (int c = 0; c < (int) CAT_COUNT; ++c) {
                s_categories[c] = true;
            }
            break;
        }
#undef TRACECATEGORY
#define TRACECATEGORY(c)                          \
    if (sv_eq_ignore_case_cstr(categories.strings[ix], #c)) { \
        s_categories[CAT_##c] = true;             \
    }
        TRACECATEGORIES(TRACECATEGORY)
#undef TRACECATEGORY
    }
}
