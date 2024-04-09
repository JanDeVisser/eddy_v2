/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <string.h>

#include <hash.h>

unsigned int hash(void const *buf, size_t size)
{
    int            hash = 5381;
    size_t         i;
    int            c;
    unsigned char *data = (unsigned char *) buf;

    for (i = 0; i < size; i++) {
        c = *data++;
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

unsigned int hashptr(void const *ptr)
{
    return hash(&ptr, sizeof(void *));
}

unsigned int hashlong(long val)
{
    return hash(&val, sizeof(long));
}

unsigned int hashdouble(double val)
{
    return hash(&val, sizeof(double));
}

unsigned int hashblend(unsigned int h1, unsigned int h2)
{
    unsigned int arr[] = { h1, h2 };
    return hash(arr, sizeof(arr));
}

unsigned int hashstr(char const *str)
{
    return hash(str, strlen(str));
}
