/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __NATIVE_H__
#define __NATIVE_H__

#include <base/resolve.h>
#include <base/rt.h>
#include <type.h>

void native_call(StringView name, size_t argc, Datum **values, Datum *ret);

#endif /* __NATIVE_H__ */
