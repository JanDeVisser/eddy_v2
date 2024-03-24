/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <app/cmake.h>
#include <app/eddy.h>
#include <base/process.h>

void cmake_cmd_build(Eddy *eddy, JSONValue unused)
{
    Process *cmake = process_create(SV("cmake", 5), "--build", sv_cstr(eddy->cmake.build_dir));
    process_background(cmake);
    while (read_pipe_expect(&cmake->out)) {
        StringView out = read_pipe_current(&cmake->out);
        printf("%.*s\n", SV_ARG(out));
    }
}
