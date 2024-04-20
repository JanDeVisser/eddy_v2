/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BASE_LOG_H
#define BASE_LOG_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include <config.h>

struct string_view;
typedef struct string_view TraceCategory;

// clang-format off
extern                            void log_init();
extern format_args(4, 5)          void _trace(char const* file_name, int line, TraceCategory category, char const *msg, ...);
extern                            void vtrace(char const *file_name, int line, TraceCategory category, char const *msg, va_list args);
extern format_args(3, 4)          void _info(char const *file_name, int line, char const *msg, ...);
extern                            void vinfo(char const *file_name, int line, char const *msg, va_list args);
extern format_args(3, 4)          void _panic(char const *file_name, int line, char const *msg, ...);
extern                            void vpanic(char const *file_name, int line, char const *msg, va_list args);
extern                            bool _log_category_on(TraceCategory category);
extern                            void log_turn_on_sv(struct string_view category);
extern                            void log_turn_off_sv(struct string_view category);
noreturn extern format_args(3, 4) void _fatal(char const *file_name, int line, char const *msg, ...);
noreturn extern                   void vfatal(char const *file_name, int line, char const *msg, va_list args);

#define fatal(msg, ...)           _fatal(__FILE_NAME__, __LINE__, msg __VA_OPT__(, ) __VA_ARGS__)
#define UNREACHABLE()             fatal("Unreachable")
#define NYI(msg, ...)             fatal("Not yet implemented in %s: " msg, __func__ __VA_OPT__(, ) __VA_ARGS__)
#define OUT_OF_MEMORY(msg, ...)   fatal("Out of memory in %s: " msg, __func__ __VA_OPT(, ) __VA_ARGS__)

#define trace(Cat, Msg, ...)      _trace(__FILE_NAME__, __LINE__, (TraceCategory) { #Cat, strlen(#Cat) }, Msg __VA_OPT__(, ) __VA_ARGS__)
#define info(Msg, ...)            _info(__FILE_NAME__, __LINE__, Msg __VA_OPT__(, ) __VA_ARGS__)
#define panic(Msg, ...)           _panic(__FILE_NAME__, __LINE__, Msg __VA_OPT__(, ) __VA_ARGS__)
#define log_turn_on(Cat)          log_turn_on_sv((TraceCategory) { #Cat, strlen(#Cat) })
#define log_turn_off(Cat)         log_turn_off_sv(TraceCategory { #Cat, strlen(#Cat) })
#define log_category_on(Cat)      _log_category_on((TraceCategory) { #Cat, strlen(#Cat) })
// clang-format on

#define assert(cond)                                                       \
    do {                                                                   \
        if (!(cond)) {                                                     \
            _fatal(__FILE_NAME__, __LINE__, "assert('%s') FAILED", #cond); \
        }                                                                  \
    } while (0)
#define assert_msg(cond, msg, ...)                                                                   \
    do {                                                                                             \
        if (!(cond)) {                                                                               \
            _fatal(__FILE_NAME__, __LINE__, "assert('%s'): " msg, #cond __VA_OPT__(, ) __VA_ARGS__); \
        }                                                                                            \
    } while (0)

#endif /* BASE_LOG_H */
