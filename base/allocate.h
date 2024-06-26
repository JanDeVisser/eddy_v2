/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __ALLOCATE_H__
#define __ALLOCATE_H__

#include <base/log.h>
#include <base/mem.h>

#ifdef STATIC_ALLOCATOR

static Allocator s_alloc = { 0 };

static Allocator get_allocator()
{
    if (!s_alloc.sentinel) {
        s_alloc = allocator_new();
    }
    return s_alloc;
}

#else

#define DECLARE_SHARED_ALLOCATOR(allocator)       \
    extern Allocator allocator##_get_allocator(); \
    static Allocator get_allocator()              \
    {                                             \
        return allocator##_get_allocator();       \
    }

#define SHARED_ALLOCATOR_IMPL(allocator)         \
    static Allocator s_alloc = { 0 };            \
    Allocator        allocator##_get_allocator() \
    {                                            \
        if (!s_alloc.sentinel) {                 \
            s_alloc = allocator_new();           \
        }                                        \
        return s_alloc;                          \
    }

#endif /* STATIC_ALLOCATOR */

static Allocator get_allocator();

static inline void *allocate(size_t size)
{
    return allocator_allocate(get_allocator(), size);
}

static inline void *array_allocate(size_t size_of_elem, size_t num_of_elems)
{
    return allocator_allocate_array(get_allocator(), size_of_elem, num_of_elems);
}

#define allocate_new(t) allocate(sizeof(t))
#define allocate_array(t, num) array_allocate(sizeof(t), num)

#define FREE_LIST(T, N)  \
    T   *allocate_##N(); \
    void free_##N(T *obj)

#define FREE_LIST_IMPL(T, N)               \
    static T *N##_free_list = NULL;        \
                                           \
    T *allocate_##N()                      \
    {                                      \
        T *ret = NULL;                     \
        if (N##_free_list) {               \
            ret = N##_free_list;           \
            N##_free_list = *((T **) ret); \
            memset(ret, 0, sizeof(T));     \
        } else {                           \
            ret = allocate_new(T);         \
        }                                  \
        return ret;                        \
    }                                      \
                                           \
    void free_##N(T *obj)                  \
    {                                      \
        *((T **) obj) = N##_free_list;     \
        N##_free_list = obj;               \
    }

#endif /* __ALLOCATE_H__ */
