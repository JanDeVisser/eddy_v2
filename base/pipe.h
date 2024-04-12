/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BASE_PIPE_H
#define BASE_PIPE_H

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
    void         *context;
    bool          debug;
};

ERROR_OR_ALIAS(ReadPipe, ReadPipe *);

typedef struct {
    int pipe[2];
    int fd;
} WritePipe;

ERROR_OR_ALIAS(WritePipe, WritePipe *);

extern ErrorOrReadPipe  read_pipe_init(ReadPipe *pipe);
extern void             read_pipe_destroy(ReadPipe *pipe);
extern ErrorOrReadPipe  read_pipe_connect(ReadPipe *p, int fd);
extern void             read_pipe_connect_parent(ReadPipe *pipe);
extern void             read_pipe_connect_child(ReadPipe *pipe, int fd);
extern void             read_pipe_close(ReadPipe *pipe);
extern bool             read_pipe_expect(ReadPipe *pipe);
extern StringView       read_pipe_current(ReadPipe *pipe);
extern ErrorOrWritePipe write_pipe_init(WritePipe *p);
extern void             write_pipe_destroy(WritePipe *pipe);
extern void             write_pipe_connect_parent(WritePipe *pipe);
extern void             write_pipe_connect_child(WritePipe *pipe, int fd);
extern void             write_pipe_close(WritePipe *pipe);
extern ErrorOrSize      write_pipe_write(WritePipe *pipe, StringView sv);
extern ErrorOrSize      write_pipe_write_chars(WritePipe *pipe, char const *buf, size_t num);

#endif /* BASE_PIPE_H */
