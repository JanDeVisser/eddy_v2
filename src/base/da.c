/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <string.h>

#include <da.h>
#include <log.h>

void da_resize(DA_void *array, size_t elem_size, size_t cap, char const *type)
{
    if (array->cap >= cap) {
        return;
    }
    trace(CAT_DA, "resiz:%15.15s:%p:%3zu:%6zu:%6zu:%6zu", type, array, elem_size, array->size, array->cap, cap);
    size_t new_cap = array->cap;
    if (new_cap == 0) {
        assert(array->size == 0);
        assert(array->elements == NULL);
        // new_cap = 15;
        new_cap = 16;
    }
    while (new_cap < cap) {
        // new_cap = (new_cap + 1) * 2 - 1;
        new_cap = new_cap * 2;
    }
    void *new_elements = NULL;
    if (array->elements == NULL) {
        // new_elements = calloc(new_cap + 1, elem_size);
        new_elements = calloc(new_cap, elem_size);
        trace(CAT_DA, "alloc:%15.15s:%p:%3zu:%6zu:%6zu:%p", type, array, elem_size, array->size, new_cap, new_elements);
    } else {
        // new_elements = realloc(array->elements - elem_size, (new_cap + 1 ) * elem_size);
        new_elements = realloc(array->elements, new_cap * elem_size);
        trace(CAT_DA, "expnd:%15.15s:%p:%3zu:%6zu:%6zu:%p", type, array, elem_size, array->size, new_cap, new_elements);
    }
    assert(new_elements);
    // array->elements = new_elements + elem_size;
    array->elements = new_elements;
    array->cap = new_cap;
}

void *da_append(DA_void *array, void *elem, size_t elem_size, char const *type)
{
    da_resize(array, elem_size, array->size + 1, type);
    memmove(array->elements + elem_size * (array->size++), elem, elem_size);
    return array->elements + elem_size * (array->size - 1);
}

void *da_element(DA_void *array, size_t ix, size_t elem_size, char const *type)
{
    assert(ix < array->size);
    return array->elements + elem_size * ix;
}

void da_free(DA_void *array, size_t elem_size, char const *type)
{
    if (array->elements) {
        // trace(CAT_DA, "free :%15.15s:%p:%3zu:%6zu:%p", type, array, elem_size, array->size, array->elements - elem_size);
        // free(array->elements - elem_size);
        trace(CAT_DA, "free :%15.15s:%p:%3zu:%6zu:%p", type, array, elem_size, array->size, array->elements);
        free(array->elements);
        array->elements = NULL;
    }
    array->size = 0;
    array->cap = 0;
}

#if 0
void *da_pop(DA_void *array, size_t elem_size, char const *type)
{
    assert(array->size > 0);
    memmove(array->elements - elem_size, array->elements + elem_size * (array->size-1), elem_size);
    --array->size;
    return array->elements - elem_size;
}

void *da_pop_front(DA_void *array, size_t elem_size, char const *type)
{
    assert(array->size > 0);
    memmove(array->elements - elem_size, array->elements, elem_size * array->size);
    --array->size;
    return array->elements - elem_size;
}
#endif

DA_IMPL_TYPE(char_ptr, char *)
DA_IMPL_TYPE(void_ptr, void *)
DA_IMPL(uint32_t)
DA_IMPL(size_t)
DA_IMPL(char)
