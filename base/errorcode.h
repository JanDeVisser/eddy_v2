/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __ERRORCODE_H__
#define __ERRORCODE_H__

typedef struct {
    int         errorno;
    char const *errorcode;
    char const *description;
} ErrorCode;

extern ErrorCode   get_errorcode(int err);
extern char const *errorcode_to_string(int err);

#endif /* __ERRORCODE_H__ */
