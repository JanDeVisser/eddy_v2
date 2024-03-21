/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <app/eddy.h>

int main(int argc, char **argv)
{
    app_initialize((AppCreate) eddy_create, argc, argv);
    app_start();
    return 0;
}
