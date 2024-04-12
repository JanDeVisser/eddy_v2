/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BASE_PROCESS_H
#define BASE_PROCESS_H

#include <base/error_or.h>
#include <base/mutex.h>
#include <base/pipe.h>
#include <base/sv.h>

typedef struct process {
    pid_t      pid;
    StringView command;
    StringList arguments;
    WritePipe  in;
    ReadPipe   out;
    ReadPipe   err;
    StringView stdout_file;
    StringView stderr_file;
} Process;

extern Process          *process_create_sl(StringView cmd, StringList *args);
extern Process          *process_vcreate(StringView cmd, va_list args);
extern Process          *_process_create(StringView cmd, ...);
extern ErrorOrInt        process_start(Process *p);
extern ErrorOrInt        process_execute(Process *p);
extern ErrorOrInt        process_wait(Process *p);
extern ErrorOrInt        process_background(Process *p);
extern ErrorOrInt        execute_sl(StringView cmd, StringList *args);
extern ErrorOrInt        _execute(StringView cmd, ...);
extern ErrorOrStringView _execute_pipe(StringView input, StringView cmd, ...);

#define process_create(cmd, ...) _process_create(cmd __VA_OPT__(, ) __VA_ARGS__, NULL)
#define execute(cmd, ...) _execute(cmd __VA_OPT__(, ) __VA_ARGS__, NULL)
#define execute_pipe(input, cmd, ...) _execute_pipe(input, cmd __VA_OPT__(, ) __VA_ARGS__, NULL)

#endif /* BASE_PROCESS_H */
