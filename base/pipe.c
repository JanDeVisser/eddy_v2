/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <unistd.h>

#include <base/errorcode.h>
#include <base/pipe.h>

static int PipeEndRead = 0;
static int PipeEndWrite = 1;

static void read_pipe_read(ReadPipe *p);
static void read_pipe_drain(ReadPipe *pipe);
static void read_pipe_newline(ReadPipe *pipe);

ErrorOrReadPipe read_pipe_init(ReadPipe *p)
{
    if (pipe(p->pipe) == -1) {
        ERROR(ReadPipe, ProcessError, errno, "pipe() failed");
    }
    RETURN(ReadPipe, p);
}

ErrorOrReadPipe read_pipe_connect(ReadPipe *p, int fd)
{
    p->fd = fd;
    fcntl(p->fd, F_SETFL, O_NONBLOCK);

    pthread_t thread;
    int       ret;
    p->condition = condition_create();
    p->current = (StringView) { 0 };
    if ((ret = pthread_create(&thread, NULL, (void *(*) (void *) ) read_pipe_read, (void *) p)) != 0) {
        fatal("Could not start IPC service thread: %s", strerror(ret));
    }
    pthread_detach(thread);
    RETURN(ReadPipe, p);
}

void read_pipe_destroy(ReadPipe *pipe)
{
    read_pipe_close(pipe);
    condition_free(pipe->condition);
    sv_free(pipe->buffer.view);
}

void read_pipe_connect_parent(ReadPipe *pipe)
{
    read_pipe_connect(pipe, pipe->pipe[PipeEndRead]);
    close(pipe->pipe[PipeEndWrite]);
}

void read_pipe_connect_child(ReadPipe *pipe, int fd)
{
    while ((dup2(pipe->pipe[PipeEndWrite], fd) == -1) && (errno == EINTR)) { }
    close(pipe->pipe[PipeEndRead]);
    close(pipe->pipe[PipeEndWrite]);
}

void read_pipe_close(ReadPipe *pipe)
{
    condition_wakeup(pipe->condition);
    if (pipe->fd >= 0) {
        close(pipe->fd);
    }
    pipe->fd = -1;
}

void read_pipe_read(ReadPipe *pipe)
{
    struct pollfd poll_fd = { 0 };
    poll_fd.fd = pipe->fd;
    poll_fd.events = POLLIN;

    while (true) {
        if (poll(&poll_fd, 1, -1) == -1) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        if (poll_fd.revents & POLLIN) {
            read_pipe_drain(pipe);
        }
        if (poll_fd.revents & POLLHUP) {
            break;
        }
    }
    read_pipe_close(pipe);
    condition_wakeup(pipe->condition);
}

#define DRAIN_SIZE (64 * 1024)

void read_pipe_drain(ReadPipe *pipe)
{
    char buffer[DRAIN_SIZE];
    condition_acquire(pipe->condition);
    while (true) {
        ssize_t count = read(pipe->fd, buffer, sizeof(buffer) - 1);
        if (count >= 0) {
            buffer[count] = 0;
            if (count > 0) {
                size_t ix = pipe->buffer.view.length;
                sb_append_chars(&pipe->buffer, buffer, count);
                if (pipe->current.ptr == NULL) {
                    pipe->current.ptr = pipe->buffer.view.ptr + ix;
                }
                pipe->current.length = (pipe->buffer.view.ptr + pipe->buffer.view.length) - pipe->current.ptr;
                if (count == sizeof(buffer) - 1) {
                    continue;
                }
            }
            break;
        }
        if (errno == EINTR) {
            continue;
        }
        panic("read_pipe_drain(): Error reading child process output: %s", errorcode_to_string(errno));
        condition_wakeup(pipe->condition);
        return;
    }

    if (pipe->on_read) {
        pipe->on_read(pipe);
    }
    condition_wakeup(pipe->condition);
}

StringView read_pipe_current(ReadPipe *pipe)
{
    condition_acquire(pipe->condition);
    StringView ret = pipe->current;
    pipe->current = (StringView) { 0 };
    condition_release(pipe->condition);
    return ret;
}

bool read_pipe_expect(ReadPipe *pipe)
{
    condition_acquire(pipe->condition);
    while (sv_empty(pipe->current)) {
        condition_sleep(pipe->condition);
        if (pipe->fd < 0) {
            return false;
        }
    }
    condition_release(pipe->condition);
    return true;
}

ErrorOrWritePipe write_pipe_init(WritePipe *p)
{
    if (pipe(p->pipe) == -1) {
        ERROR(WritePipe, ProcessError, errno, "pipe() failed");
    }
    RETURN(WritePipe, p);
}

void write_pipe_close(WritePipe *pipe)
{
    if (pipe->fd >= 0) {
        close(pipe->fd);
    }
    pipe->fd = -1;
}

void write_pipe_connect_parent(WritePipe *pipe)
{
    pipe->fd = pipe->pipe[PipeEndWrite];
    close(pipe->pipe[PipeEndRead]);
}

void write_pipe_connect_child(WritePipe *pipe, int fd)
{
    while ((dup2(pipe->pipe[PipeEndRead], fd) == -1) && (errno == EINTR)) { }
    close(pipe->pipe[PipeEndRead]);
    close(pipe->pipe[PipeEndWrite]);
}

ErrorOrSize write_pipe_write(WritePipe *pipe, StringView sv)
{
    return write_pipe_write_chars(pipe, sv.ptr, sv.length);
}

ErrorOrSize write_pipe_write_chars(WritePipe *pipe, char const *buf, size_t num)
{
    ssize_t total = { 0 };
    while (total < num) {
        ssize_t count = write(pipe->fd, buf + total, num - total);
        if (count < 0) {
            if (errno != EINTR) {
                ERROR(Size, ProcessError, errno, "Error writing to child process input");
            }
            continue;
        }
        total += count;
    }
    RETURN(Size, total);
}
