/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <unistd.h>

#include <base/http.h>
#include <base/options.h>
#include <scribble/engine.h>

int main(int argc, char **argv)
{
    char  *program_dir_or_file = NULL;
    int    scribble_param_count = 0;
    char **scribble_params = NULL;

    for (int ix = 1; ix < argc; ++ix) {
        if (!program_dir_or_file) {

            if (!strncmp(argv[ix], "--", 2) && (strlen(argv[ix]) > 2)) {
                StringView  option = sv_from(argv[ix] + 2);
                StringView  value = sv_from("true");
                char const *equals = strchr(argv[ix] + 2, '=');
                if (equals) {
                    option = (StringView) { argv[ix] + 2, equals - argv[ix] - 2 };
                    value = sv_from(equals + 1);
                }
                set_option(option, value);
            } else {
                program_dir_or_file = argv[ix];
            }
        } else {
            scribble_param_count = argc - ix;
            scribble_params = argv + ix;
        }
    }
    set_option(sv_from("scribble-dir"), sv_from(SCRIBBLE_DIR));
    log_init();

    JSONValue config = json_object();
    JSONValue stages = json_array();
    JSONValue stage = json_object();
    json_set(&config, "threaded", json_bool(has_option("threaded")));
    json_set_cstr(&stage, "name", "parse");
    json_set_cstr(&stage, "target", program_dir_or_file);
    json_set(&stage, "debug", json_bool(true));
    json_append(&stages, stage);
    stage = json_object();
    json_set_cstr(&stage, "name", "bind");
    json_set(&stage, "debug", json_bool(true));
    json_append(&stages, stage);
    stage = json_object();
    json_set_cstr(&stage, "name", "ir");
    json_set(&stage, "debug", json_bool(true));
    json_set(&stage, "list-ir", json_bool(has_option("list-ir")));
    json_append(&stages, stage);
    stage = json_object();
    json_set_cstr(&stage, "name", "generate");
    json_set(&stage, "debug", json_bool(true));
    json_set(&stage, "keep-assembly", json_bool(has_option("keep-assembly")));
    json_append(&stages, stage);
    json_set(&config, "stages", stages);
    scribble_frontend(config, frontend_message_handler);
}
