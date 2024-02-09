/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <sv.h>

#include <allocate.h>

DECLARE_SHARED_ALLOCATOR(sv)

DA_IMPL_ELEMENTS(StringView, strings)

StringList sl_create()
{
    StringList ret = { 0 };
    return ret;
}

void sl_free(StringList *sl)
{
    assert(sl);
    for (size_t ix = 0; ix < sl->size; ++ix) {
        sv_free(sl->strings[ix]);
    }
}

StringList sl_copy(StringList *sl)
{
    assert(sl);
    StringList ret = sl_create();
    for (size_t ix = 0; ix < sl->size; ++ix) {
        sl_push(&ret, sl->strings[ix]);
    }
    return ret;
}

StringList *sl_push(StringList *sl, StringView sv)
{
    da_append_StringView(sl, sv);
    return sl;
}

StringList *sl_extend(StringList *sl, StringList *with)
{
    assert(sl);
    assert(with);
    for (size_t ix = 0; ix < with->size; ++ix) {
        da_append_StringView(sl, with->strings[ix]);
    }
    return sl;
}

StringView sl_pop(StringList *sl)
{
    if (sl_empty(sl)) {
        return sv_null();
    }
    return sl->strings[sl->size--];
}

StringView sl_pop_front(StringList *sl)
{
    if (sl_empty(sl)) {
        return sv_null();
    }
    StringView ret = sl->strings[0];
    memcpy(sl->strings, sl->strings + 1, (sl->size-1) * sizeof(StringView));
    --sl->size;
    return ret;
}

StringList sl_split(StringList *sl, size_t at)
{
    if (sl->size <= at) {
        return (StringList) {0};
    }
    StringList ret = {0};
    da_resize_StringView(&ret, sl->size - at);
    memcpy(ret.strings, sl->strings + at, (sl->size-at) * sizeof(StringView));
    ret.size = sl->size - at;
    sl->size = at;
    return ret;
}

StringView sl_join(StringList *sl, StringView sep)
{
    StringBuilder sb = sb_create();
    sb_append_list(&sb, sl, sep);
    return sb.view;
}

StringView sl_front(StringList *sl)
{
    if (sl_empty(sl)) {
        return sv_null();
    }
    return sl->strings[0];
}

StringView sl_back(StringList *sl)
{
    if (sl_empty(sl)) {
        return sv_null();
    }
    return sl->strings[sl->size - 1];
}

bool sl_empty(StringList *sl)
{
    return !sl || (sl->size == 0);
}

size_t sl_size(StringList *sl)
{
    return (sl) ? sl->size : 0;
}
