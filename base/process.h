/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <base/error_or.h>
#include <base/mutex.h>
#include <base/sv.h>

typedef struct _read_pipe ReadPipe;
typedef void (*OnPipeRead)(ReadPipe *);

struct _read_pipe {
    int           pipe[2];
    int           fd;
    StringBuilder buffer;
    StringView    current;
    Condition     condition;
    OnPipeRead    on_read;
    bool          debug;
};

ERROR_OR_ALIAS(ReadPipe, ReadPipe *);

typedef struct {
    int pipe[2];
    int fd;
} WritePipe;

ERROR_OR_ALIAS(WritePipe, WritePipe *);

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

extern ErrorOrReadPipe   read_pipe_init(ReadPipe *pipe);
extern void              read_pipe_destroy(ReadPipe *pipe);
extern void              read_pipe_connect_parent(ReadPipe *pipe);
extern void              read_pipe_connect_child(ReadPipe *pipe, int fd);
extern void              read_pipe_close(ReadPipe *pipe);
extern void              read_pipe_read(ReadPipe *p);
extern bool              read_pipe_expect(ReadPipe *pipe);
extern StringView        read_pipe_current(ReadPipe *pipe);
extern ErrorOrWritePipe  write_pipe_init(WritePipe *p);
extern void              write_pipe_destroy(WritePipe *pipe);
extern void              write_pipe_connect_parent(WritePipe *pipe);
extern void              write_pipe_connect_child(WritePipe *pipe, int fd);
extern void              write_pipe_close(WritePipe *pipe);
extern ErrorOrSize       write_pipe_write(WritePipe *pipe, StringView sv);
extern ErrorOrSize       write_pipe_write_chars(WritePipe *pipe, char const *buf, size_t num);
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

#endif /* __PROCESS_H__ */
