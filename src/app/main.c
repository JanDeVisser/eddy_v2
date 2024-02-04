/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <raylib.h>

#include <allocate.h>

#include <eddy.h>
#include <widget.h>

DECLARE_SHARED_ALLOCATOR(eddy);
SHARED_ALLOCATOR_IMPL(eddy);

int main(int argc, char **argv)
{
    app_initialize((App *) &eddy, (AppCreate) eddy_create, argc, argv);
    app_start();
    return 0;
}
