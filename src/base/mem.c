/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include <log.h>
#include <mem.h>

static size_t const ARENA_SZ = 10 * 1024 * 1024;

Arena *arena_initialize(Arena *arena, size_t size);
void   arena_free(Arena *arena);
void  *arena_allocate(Arena *arena, size_t size);

void *malloc_fatal(size_t size, char const *where, ...)
{
    void *ret = malloc(size);
    if (!ret) {
        va_list args;
        va_start(args, where);
        char strbuf[256];
        vsnprintf(strbuf, 255, where, args);
        strbuf[255] = 0;
        vfatal("Out of memory: %s", strbuf);
        va_end(args);
    }
    memset(ret, 0, size);
    return ret;
}

Arena *arena_initialize(Arena *arena, size_t size)
{
    arena->arena = NULL;
    arena->size = size;
    arena->ptr = 0;
    return arena;
}

void arena_free(Arena *arena)
{
    free(arena->arena);
    arena->arena = NULL;
    arena->ptr = 0;
}

void *arena_allocate(Arena *arena, size_t size)
{
    assert(arena->size > 0);
    if (size > arena->size - arena->ptr) {
        return NULL;
    }
    if (arena->arena == NULL) {
        assert(arena->ptr == 0);
        arena->arena = malloc_fatal(arena->size, __func__);
    }
    void *ret = arena->arena + arena->ptr;
    memset(arena->arena + arena->ptr, 0, arena->size - arena->ptr);
    arena->ptr += size;
    return ret;
}

Allocator *allocator_new_with_size(size_t arena_size)
{
    Allocator *ret = MALLOC(Allocator);
    allocator_init(ret, arena_size);
    return ret;
}

Allocator *allocator_new()
{
    return allocator_new_with_size(ARENA_SZ);
}

void allocator_init(Allocator *alloc, size_t arena_size)
{
    alloc->arena_size = arena_size;
    alloc->arenas = MALLOC(Arena);
    alloc->cap = 1;
    alloc->num = 1;
    arena_initialize(alloc->arenas, arena_size);
}

void allocator_reset(Allocator *alloc)
{
    for (size_t ix = 0; ix < alloc->num; ++ix) {
        arena_free(alloc->arenas + ix);
    }
    alloc->num = 0;
    alloc->num_states = 0;
}

void *allocator_allocate(Allocator *alloc, size_t size)
{
    if (alloc->arena_size == 0) {
        allocator_init(alloc, ARENA_SZ);
    }
    assert(alloc->num > 0);
    void *ret = NULL;

    size_t min_arena = 0;
    if (alloc->num_states > 0) {
        min_arena = alloc->states[alloc->num_states - 1].index;
    }

    for (size_t ix = min_arena; ix < alloc->num; ++ix) {
        ret = arena_allocate(alloc->arenas + ix, size);
        if (ret) {
            trace(CAT_MEM, "M:0x%08llx:%5zu:0x%08llx", (uint64_t) alloc, size, (uint64_t) ret);
            return ret;
        }
    }

    if (alloc->num == alloc->cap) {
        alloc->cap *= 2;
        Arena *old = alloc->arenas;
        alloc->arenas = realloc(alloc->arenas, alloc->cap * sizeof(Arena));
        assert(alloc->states);
        if (alloc->arenas != old) {
            free(old);
        }
        memset(alloc->arenas + alloc->num, 0, (alloc->cap - alloc->num) * sizeof(Arena));
    }
    size_t arena_size = alloc->arena_size;
    if (size > arena_size) {
        arena_size = size;
    }
    arena_initialize(alloc->arenas + alloc->num, arena_size);
    ret = arena_allocate(alloc->arenas + alloc->num, size);
    trace(CAT_MEM, "M:0x%08llx:%5zu:0x%08llx", (uint64_t) alloc, size, (uint64_t) ret);
    assert(ret != NULL);
    ++alloc->num;
    return ret;
}

void *allocator_allocate_array(Allocator *alloc, size_t size, size_t num)
{
    return allocator_allocate(alloc, size * num);
}

void allocator_save(Allocator *alloc)
{
    AllocatorState state = { 0 };
    if (alloc->num != 0) {
        state.index = alloc->num - 1;
        state.ptr = alloc->arenas[alloc->num - 1].ptr;
    }
    if (alloc->num_states == alloc->cap_states) {
        alloc->cap_states = (alloc->cap_states) ? alloc->cap_states * 2 : 2;
        AllocatorState *old = alloc->states;
        alloc->states = realloc(alloc->states, alloc->cap_states * sizeof(AllocatorState));
        assert(alloc->states);
        if (alloc->states != old) {
            free(old);
        }
        memset(alloc->states + alloc->num_states, 0, (alloc->cap_states - alloc->num_states) * sizeof(AllocatorState));
    }
    alloc->states[alloc->num_states++] = state;
}

void allocator_release(Allocator *alloc)
{
    assert(alloc->num_states > 0);
    if (alloc->num > 0) {
        AllocatorState state = alloc->states[--alloc->num_states];
        for (size_t ix = alloc->num - 1; ix > state.index; --ix) {
            arena_free(alloc->arenas + ix);
            --alloc->num;
        }
        if (alloc->num == 0) {
            assert(state.index == 0 && state.ptr == 0);
            return;
        }
        assert(state.index == alloc->num - 1);
        alloc->arenas[state.index].ptr = state.ptr;
    }
}

static Allocator s_alloc = { 0 };

void *mem_allocate(size_t size)
{
    return allocator_allocate(&s_alloc, size);
}

void *mem_allocate_array(size_t count, size_t element_size)
{
    return mem_allocate(count * element_size);
}

void mem_destroy()
{
    allocator_reset(&s_alloc);
}

void mem_save()
{
    allocator_save(&s_alloc);
}

void mem_release()
{
    allocator_release(&s_alloc);
}
