/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h>

#ifndef __DA_H__
#define __DA_H__

#define DIA_ELEMENTS(T, E) \
    size_t size;           \
    size_t cap;            \
    T     *E

#define DIA(T) DIA_ELEMENTS(T, elements)

#define DIA_APPEND_ELEMENT(T, E, obj, elem)                             \
    do {                                                                \
        if ((obj)->size == obj->cap) {                                  \
            if ((obj)->cap == 0) {                                      \
                (obj)->E = array_allocate(sizeof(T), 4);                \
                (obj)->cap = 4;                                         \
            } else {                                                    \
                size_t new_cap = (obj)->cap;                            \
                do {                                                    \
                    new_cap *= 2;                                       \
                } while (new_cap < (obj)->size);                        \
                T *new_elements = array_allocate(sizeof(T), new_cap);   \
                memcpy(new_elements, (obj)->E, (obj)->cap * sizeof(T)); \
                (obj)->E = new_elements;                                \
                (obj)->cap = new_cap;                                   \
            }                                                           \
        }                                                               \
        (obj)->E[(obj)->size++] = (elem);                               \
    } while (0)

#define DIA_APPEND(T, obj, elem) DIA_APPEND_ELEMENT(T, elements, obj, elem)

typedef struct {
    size_t size;
    size_t cap;
    void  *elements;
} DA_void;

#define DA_FUNCTIONS_TYPE(T, S)                    \
    void da_resize_##T(DA_##T *array, size_t cap); \
    S   *da_append_##T(DA_##T *array, S elem);     \
    S   *da_element_##T(DA_##T *array, size_t ix);

#define DA_FUNCTIONS(T) DA_FUNCTIONS_TYPE(T, T)

#define DA_STRUCT_ELEMENTS(T, S, E) \
    typedef struct _da_##T {        \
        size_t size;                \
        size_t cap;                 \
        S     *E;                   \
    } DA_##T;                       \
    DA_FUNCTIONS_TYPE(T, S)

extern void  da_resize(DA_void *array, size_t elem_size, size_t cap);
extern void *da_append(DA_void *array, void *elem, size_t elem_size);
extern void *da_element(DA_void *array, size_t ix, size_t elem_size);

#define DA_VOID(T) \
    typedef DA_void DA_##T;
#define DA_VOID_WITH_NAME(T, name) \
    typedef DA_void DA_##T;        \
    typedef DA_##T  name;

#define DA_IMPL_TYPE(T, S)                                           \
    void da_resize_##T(DA_##T *array, size_t cap)                    \
    {                                                                \
        da_resize((DA_void *) array, sizeof(S), cap);                \
    }                                                                \
    S *da_append_##T(DA_##T *array, S elem)                          \
    {                                                                \
        return (S *) da_append((DA_void *) array, &elem, sizeof(S)); \
    }                                                                \
    S *da_element_##T(DA_##T *array, size_t ix)                      \
    {                                                                \
        return (S *) da_element((DA_void *) array, ix, sizeof(S));   \
    }
#define DA_IMPL(T) DA_IMPL_TYPE(T, T)

#define DA_STRUCT(T, S) DA_STRUCT_ELEMENTS(T, S, elements)
#define DA(T) DA_STRUCT(T, T)
#define DA_STRUCT_WITH_NAME(T, S, name) \
    DA_STRUCT(T, S)                     \
    typedef DA_##T name
#define DA_WITH_NAME(T, name) DA_STRUCT_WITH_NAME(T, T, name)
#define DA_ELEMENTS(T, E) DA_STRUCT_ELEMENTS(T, T, E)

#define DA_IMPL_ELEMENTS(T, E) DA_IMPL(T)

DA_STRUCT(void_ptr, void *);
DA_STRUCT(char_ptr, char *);

#endif /* __DA_H__ */
