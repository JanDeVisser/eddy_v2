/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h>

#include <base/options.h>
#include <scribble/engine.h>
#include <scribble/type.h>


int main(int argc, char **argv)
{
    StringView socket_path = {0};
    for (int ix = 1; ix < argc; ++ix) {
        if ((strlen(argv[ix]) > 2) && !strncmp(argv[ix], "--", 2)) {
            StringView  option = sv_from(argv[ix] + 2);
            StringView  value = sv_from("true");
            char const *equals = strchr(argv[ix] + 2, '=');
            if (equals) {
                option = (StringView) { argv[ix] + 2, equals - argv[ix] - 2 };
                value = sv_from(equals + 1);
            }
            set_option(option, value);
        } else {
            socket_path = sv_from(argv[ix]);
        }
    }
    set_option(sv_from("scribble-dir"), sv_from(SCRIBBLE_DIR));
    if (sv_empty(socket_path)) {
        exit(1);
    }
    log_init();
    type_registry_init();
    return scribble_backend(socket_path, false);
}
