/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h>

#ifndef __MEM_H__
#define __MEM_H__

typedef struct {
    size_t index;
    size_t ptr;
} AllocatorState;

typedef struct arena {
    size_t size;
    char  *arena;
    size_t ptr;
} Arena;

typedef struct allocator {
    size_t          cap;
    size_t          num;
    Arena          *arenas;
    size_t          arena_size;
    size_t          cap_states;
    size_t          num_states;
    AllocatorState *states;
} Allocator;

void      *malloc_fatal(size_t size, char const *where, ...);
Allocator *allocator_new_with_size(size_t arena_size);
Allocator *allocator_new();
void       allocator_init(Allocator *alloc, size_t a);
void      *allocator_allocate(Allocator *alloc, size_t size);
void      *allocator_allocate_array(Allocator *alloc, size_t size, size_t num);
void       allocator_reset(Allocator *alloc);
void       allocator_save(Allocator *alloc);
void       allocator_release(Allocator *alloc);
void      *mem_allocate(size_t size);
void      *mem_allocate_array(size_t count, size_t element_size);
void       mem_free();
void       mem_save();
void       mem_release();

#define MALLOC(t) ((t *) malloc_fatal(sizeof(t), "allocating " #t))
#define MALLOC_ARR(t, num) ((t *) malloc_fatal((num * sizeof(t)), "allocating array of %d " #t "s", num));
#define allocator_alloc_new(alloc, T) allocator_allocate(alloc, sizeof(T))
#define allocator_alloc_array(alloc, T, num) allocator_allocate_array(alloc, sizeof(T), num)

#endif /* __MEM_H__ */
