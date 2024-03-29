/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __MEM_H__
#define __MEM_H__

#include <stdlib.h>

typedef struct allocator {
    size_t sentinel;
} Allocator;

void     *malloc_fatal(size_t size, char const *where, ...);
Allocator allocator_new();
void     *allocator_allocate(Allocator alloc, size_t size);
void     *allocator_allocate_array(Allocator alloc, size_t size, size_t num);
void     *mem_allocate(size_t size);
void     *mem_allocate_array(size_t count, size_t element_size);
void      mem_free();

#define MALLOC(t) ((t *) malloc_fatal(sizeof(t), "allocating " #t))
#define MALLOC_ARR(t, num) ((t *) malloc_fatal((num * sizeof(t)), "allocating array of %d " #t "s", num));
#define allocator_alloc_new(alloc, T) allocator_allocate(alloc, sizeof(T))
#define allocator_alloc_array(alloc, T, num) allocator_allocate_array(alloc, sizeof(T), num)

#endif /* __MEM_H__ */
