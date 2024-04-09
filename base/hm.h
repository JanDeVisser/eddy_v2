/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BASE_HM_H
#define BASE_HM_H

#include <stdlib.h>

#include <base/sv.h>

typedef unsigned int (*HashFnc)(void const *);

typedef struct {
    StringView key;
    StringView value;
} HashEntry;

DA_WITH_NAME(HashEntry, HashEntries);
DA_WITH_NAME(HashEntries, HashBuckets);

typedef struct {
    size_t      size;
    HashBuckets buckets;
} HashMap;

extern void               hm_put(HashMap *hm, StringView key, StringView value);
extern bool               hm_has(HashMap *hm, StringView key);
extern OptionalStringView hm_get(HashMap *hm, StringView key);

#endif /* BASE_HM_H */
