/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <pwd.h>
#include <unistd.h>

#include <io.h>
#include <process.h>

int main(int argc, char **argv)
{
    struct passwd *pw = getpwuid(getuid());
    chdir(pw->pw_dir);

    StringView text = MUST(StringView, read_file_by_name(sv_from("projects/eddy_v2/src/app/editor.c")));
    StringView formatted = MUST(StringView, execute_pipe(
        text,
        sv_from("clang-format"),
        "--assume-filename=projects/eddy_v2/src/app/editor.c"));
    printf("%.*s\n", SV_ARG(formatted));
    return 0;
}
