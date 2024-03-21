/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __LOG_H__
#define __LOG_H__

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include <config.h>

#define TRACECATEGORIES(S) \
    S(NONE)                \
    S(DA)                  \
    S(LIB)                 \
    S(THREAD)              \
    S(PROCESS)             \
    S(JSON)                \
    S(MEM)                 \
    S(SV)                  \
    S(PARSE)               \
    S(BIND)                \
    S(IR)                  \
    S(EXECUTE)             \
    S(COMPILE)             \
    S(HTTP)                \
    S(IPC)                 \
    S(LSP)                 \
    S(XML)                 \
    S(TEMPLATE)            \
    S(EDIT)

typedef enum trace_category {
#undef TRACECATEGORY
#define TRACECATEGORY(cat) CAT_##cat,
    TRACECATEGORIES(TRACECATEGORY)
#undef TRACECATEGORY
        CAT_COUNT
} TraceCategory;

struct string_view;

// clang-format off
extern                            void log_init();
extern                            void trace_nl(TraceCategory category);
extern format_args(2, 3)          void trace_nonl(TraceCategory category, char const *msg, ...);
extern                            void vtrace_nonl(TraceCategory category, char const *msg, va_list args);
extern format_args(2, 3)          void trace(TraceCategory category, char const *msg, ...);
extern                            void vtrace(TraceCategory category, char const *msg, va_list args);
extern format_args(1, 2)          void panic(char const *msg, ...);
extern                            void vpanic(char const *msg, va_list args);
extern                            bool log_category_on(TraceCategory category);
extern                            void log_turn_on(TraceCategory category);
extern                            void log_turn_off(TraceCategory category);
extern                            void log_turn_on_sv(struct string_view category);
extern                            void log_turn_off_sv(struct string_view category);
noreturn extern format_args(1, 2) void _fatal(char const *msg, ...);
noreturn extern                   void vfatal(char const *msg, va_list args);

#define fatal(msg, ...)           _fatal("%s:%d: " msg, __FILE_NAME__, __LINE__ __VA_OPT__(, ) __VA_ARGS__)
#define UNREACHABLE()             fatal("Unreachable")
#define NYI(msg, ...)             fatal("Not yet implemented in %s: " msg, __func__ __VA_OPT__(, ) __VA_ARGS__)
#define OUT_OF_MEMORY(msg, ...)   fatal("Out of memory in %s: " msg, __func__ __VA_OPT(, ) __VA_ARGS__)
// clang-format on

#define assert(cond)                                                             \
    do {                                                                         \
        if (!(cond)) {                                                           \
            fatal("%s:%d: assert('%s') FAILED", __FILE_NAME__, __LINE__, #cond); \
        }                                                                        \
    } while (0)
#define assert_msg(cond, msg, ...)                                                                         \
    do {                                                                                                   \
        if (!(cond)) {                                                                                     \
            fatal("%s:%d: assert('%s'): " msg, __FILE_NAME__, __LINE__, #cond __VA_OPT__(, ) __VA_ARGS__); \
        }                                                                                                  \
    } while (0)

#endif // __LOG_H__
