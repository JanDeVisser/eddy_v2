/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <base/hm.h>

typedef struct {
    unsigned int bucket;
    size_t       entry;
} EntryIndex;

OPTIONAL(EntryIndex);

void initialize_buckets(HashBuckets *buckets, size_t num_buckets)
{
    da_resize_HashEntries(buckets, num_buckets);
    buckets->size = 0;
    for (size_t ix = 0; ix < num_buckets; ++ix) {
        da_append_HashEntries(buckets, (HashEntries) { 0 });
    }
}

void rehash_map(HashMap *hm)
{
    HashBuckets new_buckets = { 0 };
    initialize_buckets(&new_buckets, hm->buckets.size * 2);
    for (size_t bix = 0; bix < hm->buckets.size; ++bix) {
        HashEntries *entries = hm->buckets.elements + bix;
        for (size_t eix = 0; eix < entries->size; ++eix) {
            HashEntry    entry = entries->elements[eix];
            unsigned int h = sv_hash(&entry.key) % new_buckets.size;
            da_append_HashEntry(new_buckets.elements + h, entry);
        }
    }
    da_free_HashEntries(&hm->buckets);
    hm->buckets = new_buckets;
}

OptionalEntryIndex find_entry(HashMap *hm, StringView key)
{
    unsigned int h = sv_hash(&key) % hm->buckets.size;
    HashEntries *bucket = hm->buckets.elements + h;
    for (size_t ix = 0; ix < bucket->size; ++ix) {
        if (sv_eq(bucket->elements[ix].key, key)) {
            RETURN_VALUE(EntryIndex, ((EntryIndex) { h, ix }));
        }
    }
    RETURN_EMPTY(EntryIndex);
}

void hm_put(HashMap *hm, StringView key, StringView value)
{
    if (hm->buckets.size == 0) {
        initialize_buckets(&hm->buckets, 4);
    }
    if ((float) (hm->size + 1) / (float) (hm->buckets.size) > 0.75) {
        rehash_map(hm);
        da_resize_HashEntries(&hm->buckets, 2 * hm->buckets.size);
    }
    unsigned int bucket = sv_hash(&key) % hm->buckets.size;
    da_append_HashEntry(hm->buckets.elements + bucket, (HashEntry) { .key = key, .value = value });
}

bool hm_has(HashMap *hm, StringView key)
{
    OptionalEntryIndex entry_maybe = find_entry(hm, key);
    return entry_maybe.has_value;
}

OptionalStringView hm_get(HashMap *hm, StringView key)
{
    OptionalEntryIndex entry_maybe = find_entry(hm, key);
    if (entry_maybe.has_value) {
        HashEntries *bucket = hm->buckets.elements + entry_maybe.value.bucket;
        HashEntry *entry = bucket->elements + entry_maybe.value.entry;
        RETURN_VALUE(StringView, entry->value);
    }
    RETURN_EMPTY(StringView);
}
