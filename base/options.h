/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#include <base/sv.h>

typedef struct option_list {
    StringView           option;
    StringView           value;
    struct option_list *next;
} OptionList;

extern void       set_option(StringView option, StringView value);
extern StringView get_option(StringView option);
extern StringList get_option_values(StringView option);

#define has_option(opt) (!sv_empty(get_option(sv_from(opt))))
#define OPT_DEBUG (!sv_empty(get_option(sv_from("debug"))))
#define OPT_EMULATE (!sv_empty(get_option(sv_from("emulate"))))
#define OPT_TRACE (!sv_empty(get_option(sv_from("trace"))))
#define OPT_GRAPH (!sv_empty(get_option(sv_from("graph"))))
#define OPT_RUN (!sv_empty(get_option(sv_from("run"))))
#define OPT_STATIC (!sv_empty(get_option(sv_from("static"))))

#endif /* __OPTIONS_H__ */
