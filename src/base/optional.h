/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __OPTIONAL_H__
#define __OPTIONAL_H__

#include <da.h>
#include <error_or.h>

#define OPTIONAL_ALIAS(T, alias)                                        \
    typedef struct optional_##alias {                                   \
        bool has_value;                                                 \
        T    value;                                                     \
    } Optional##alias;                                                  \
    static inline Optional##alias Optional##alias##_create(T value)     \
    {                                                                   \
        return (Optional##alias) { .has_value = true, .value = value }; \
    }                                                                   \
    static inline Optional##alias Optional##alias##_empty()             \
    {                                                                   \
        return (Optional##alias) { .has_value = false };                \
    }

#define OPTIONAL(T) OPTIONAL_ALIAS(T, T)

#define MUST_OPTIONAL(T, expr)                                          \
    ({                                                                  \
        Optional##T T##_maybe = (expr);                                 \
        if (!T##_maybe.has_value) {                                     \
            fatal("Optional expression '%s' (%s:%d) returned no value", \
                #expr, __FILE_NAME__, __LINE__);                        \
        }                                                               \
        T##_maybe.value;                                                \
    })

#define TRY_OPTIONAL(T, expr, E, CAT, MSG, ...)               \
    ({                                                        \
        Optional##T T##_maybe = (expr);                       \
        if (!T##_maybe.has_value) {                           \
            ERROR(E, CAT, 0, MSG __VA_OPT__(, ) __VA_ARGS__); \
        }                                                     \
        T##_maybe.value;                                      \
    })

#define RETURN_VALUE(T, exp) \
    return (Optional##T) { .has_value = true, .value = (exp) }
#define RETURN_EMPTY(T) \
    return (Optional##T) { .has_value = false }

OPTIONAL_ALIAS(int, Int)
OPTIONAL_ALIAS(uint32_t, UInt32)
OPTIONAL_ALIAS(uint64_t, UInt64)
OPTIONAL_ALIAS(int64_t, Int64)
OPTIONAL_ALIAS(bool, Bool)
typedef OptionalUInt32 OptionalUInt;

#define PAIR_WITH_NAME_AND_PLURAL(T1, T2, N, N_PLURAL) \
    typedef struct {                                   \
        T1 key;                                        \
        T2 value;                                      \
    } N;                                               \
    DA_WITH_NAME(N, N_PLURAL)

#define PAIR_WITH_NAME(T1, T2, N) PAIR_WITH_NAME_AND_PLURAL(T1, T2, N, N##s)
#define PAIR(T1, T2) PAIR_WITH_NAME(T1, T2, T1##_##T2##_pair)

#endif /* __OPTIONAL_H__ */
