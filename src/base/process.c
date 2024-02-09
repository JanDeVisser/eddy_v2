/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#define STATIC_ALLOCATOR
#include <allocate.h>
#include <log.h>
#include <process.h>
#include <pthread.h>
#include <sys/poll.h>

static int PipeEndRead = 0;
static int PipeEndWrite = 1;

void read_pipe_drain(ReadPipe *pipe);
void read_pipe_newline(ReadPipe *pipe);

ErrorOrReadPipe read_pipe_init(ReadPipe *p)
{
    if (pipe(p->pipe) == -1) {
        ERROR(ReadPipe, ProcessError, errno, "pipe() failed");
    }
    p->condition = condition_create();
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
    pipe->fd = pipe->pipe[PipeEndRead];
    close(pipe->pipe[PipeEndWrite]);
    fcntl(pipe->fd, F_SETFL, O_NONBLOCK);

    pthread_t      thread;
    int            ret;
    if ((ret = pthread_create(&thread, NULL, (void*(*)(void*)) read_pipe_read, (void *) pipe)) != 0) {
        fatal("Could not start IPC service thread: %s", strerror(ret));
    }
    pthread_detach(thread);
}

void read_pipe_connect_child(ReadPipe *pipe, int fd)
{
    while ((dup2(pipe->pipe[PipeEndWrite], fd) == -1) && (errno == EINTR)) { }
    close(pipe->pipe[PipeEndRead]);
    close(pipe->pipe[PipeEndWrite]);
}

void read_pipe_close(ReadPipe *pipe)
{
    if (sv_not_empty(pipe->current_line)) {
        read_pipe_newline(pipe);
    }
    condition_wakeup(pipe->condition);
    if (pipe->fd >= 0) {
        close(pipe->fd);
    }
    pipe->fd = -1;
}

void read_pipe_read(ReadPipe *pipe)
{
    struct pollfd poll_fd = {0};
    poll_fd.fd = pipe->fd;
    poll_fd.events = POLLIN;

    while (true) {
        // printf("Polling fd %d\n", pipe->fd);
        if (poll(&poll_fd, 1, -1) == -1) {
            printf("poll() failed on fd %d: %s\n", pipe->fd, strerror(errno));
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        if (poll_fd.revents & POLLIN) {
            // printf("Input ready on fd %d\n", pipe->fd);
            read_pipe_drain(pipe);
        } else {
            printf("spurious poll() return on fd %d\n", pipe->fd);
        }
    }
    read_pipe_close(pipe);
};

void read_pipe_drain(ReadPipe *pipe)
{
    char buffer[4096];
    condition_acquire(pipe->condition);
    for (ssize_t count = read(pipe->fd, buffer, sizeof(buffer) - 1); count != 0; count = read(pipe->fd, buffer, sizeof(buffer) - 1)) {
        if (count > 0) {
            size_t ix = pipe->buffer.view.length;
            sb_append_chars(&pipe->buffer, buffer, count);
            bool prev_was_cr = false;
            for (; ix < pipe->buffer.view.length; ++ix) {
                int ch = pipe->buffer.view.ptr[ix];
                switch (ch) {
                case '\r':
                    read_pipe_newline(pipe);
                    prev_was_cr = true;
                    break;
                case '\n':
                    if (!prev_was_cr) {
                        read_pipe_newline(pipe);
                    }
                    prev_was_cr = true;
                    break;
                default:
                    if (pipe->current_line.ptr == NULL) {
                        pipe->current_line.ptr = pipe->buffer.view.ptr + ix;
                        pipe->current_line.length = 0;
                    }
                    ++pipe->current_line.length;
                    prev_was_cr = false;
                }
            }
            continue;
        }
        switch (errno) {
        case EINTR:
            continue;
        case EBADF:
        case EAGAIN:
            goto exit_loop;
        default:
            fprintf(stderr, "Error reading child process output: %s\n", strerror(errno));
            goto exit_loop;
        }
    }
exit_loop:
    if (sv_not_empty(pipe->current_line)) {
        read_pipe_newline(pipe);
    }
    condition_wakeup(pipe->condition);
}

void read_pipe_newline(ReadPipe *pipe)
{
    sl_push(&pipe->lines, pipe->current_line);
    pipe->current_line = (StringView) {0};
}

StringList read_pipe_lines(ReadPipe *pipe)
{
    condition_acquire(pipe->condition);
    StringList ret = pipe->lines;
    pipe->lines = (StringList) {0};
    condition_release(pipe->condition);
    return ret;
}

void read_pipe_expect(ReadPipe *pipe)
{
    condition_acquire(pipe->condition);
    while (sl_empty(&pipe->lines)) {
        condition_sleep(pipe->condition);
    }
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

ErrorOrSize write_pipe_write_chars(WritePipe *pipe, char const* buf, size_t num)
{
    ssize_t count;
    // printf("Writing %zu bytes to fd %d\n", num, pipe->fd);
    while (true) {
        count = write(pipe->fd, buf, num);
        if (count >= 0)
            break;
        if (errno != EINTR)
            ERROR(Size, ProcessError, errno, "Error writing to child process input");
    }
    RETURN(Size, count);
}

static void process_dump(Process *p)
{
    trace(CAT_LIB, "Command: '%.*s' #arguments: %zu", SV_ARG(p->command), p->arguments.size);
    for (size_t ix = 0; ix < p->arguments.size; ++ix) {
        trace(CAT_LIB, "Arg #%zu: '%.*s'", ix, SV_ARG(p->arguments.strings[ix]));
    }
}

Process *process_create_sl(StringView cmd, StringList *args)
{
    Process *p = allocate_new(Process);
    p->command = cmd;
    p->arguments = sl_copy(args);
    return p;
}

Process *process_vcreate(StringView cmd, va_list args)
{
    StringList sl_args = sl_create();
    for (char const *arg = va_arg(args, char const *); arg; arg = va_arg(args, char const *)) {
        sl_push(&sl_args, sv_copy_cstr(arg));
    }
    return process_create_sl(cmd, &sl_args);
}

Process *_process_create(StringView cmd, ...)
{
    va_list args;
    va_start(args, cmd);
    Process *ret = process_vcreate(cmd, args);
    va_end(args);
    return ret;
}

ErrorOrInt process_start(Process *p)
{
    size_t sz = p->arguments.size;
    char **argv = allocate_array(char *, sz + 2);
    argv[0] = (char *) sv_cstr(p->command);
    for (size_t ix = 0u; ix < sz; ++ix) {
        argv[ix + 1] = (char *) sv_cstr(p->arguments.strings[ix]);
    }
    argv[sz + 1] = NULL;
    StringView args = sl_join(&p->arguments, sv_from(" "));
    printf("[CMD] %.*s %.*s\n", SV_ARG(p->command), SV_ARG(args));
    sv_free(args);

    signal(SIGCHLD, SIG_IGN);
    TRY_TO(WritePipe, Int, write_pipe_init(&p->in));
    TRY_TO(ReadPipe, Int, read_pipe_init(&p->out));
    TRY_TO(ReadPipe, Int, read_pipe_init(&p->err));

    pid_t pid = fork();
    if (pid == -1) {
        ERROR(Int, ProcessError, errno, "fork() failed");
    }
    p->pid = pid;
    if (pid == 0) {
        write_pipe_connect_child(&p->in, STDIN_FILENO);
        read_pipe_connect_child(&p->out, STDOUT_FILENO);
        read_pipe_connect_child(&p->err, STDERR_FILENO);
        execvp(argv[0], argv);
        printf("execvp(%.*s) failed: %s\n", SV_ARG(p->command), strerror(errno));
        exit(1);
    }
    write_pipe_connect_parent(&p->in);
    read_pipe_connect_parent(&p->out);
    read_pipe_connect_parent(&p->err);
    if (!sv_is_cstr(p->command)) {
        free(argv[0]);
    }
    for (size_t ix = 0; ix < sz; ++ix) {
        if (!sv_is_cstr(p->arguments.strings[ix])) {
            free(argv[ix+1]);
        }
    }
    RETURN(Int, pid);
}

ErrorOrInt process_wait(Process *p)
{
    if (p->pid == 0) {
        RETURN(Int, 0);
    }
    int exit_code;
    if (waitpid(p->pid, &exit_code, 0) == -1 && errno != ECHILD && errno != EINTR) {
        ERROR(Int, ProcessError, errno, "waitpid() failed");
    }
    p->pid = 0;
    write_pipe_close(&p->in);
    if (!WIFEXITED(exit_code)) {
        ERROR(Int, ProcessError, errno, "Child program %.*s crashed due to signal %d", SV_ARG(p->command), WTERMSIG(exit_code));
    }
    RETURN(Int, WEXITSTATUS(exit_code));
}

ErrorOrInt process_background(Process *p)
{
    return process_start(p);
}

ErrorOrInt process_execute(Process *p)
{
    TRY(Int, process_start(p));
    return process_wait(p);
}

ErrorOrInt execute_sl(StringView cmd, StringList *args)
{
    Process *p = process_create_sl(cmd, args);
    return process_execute(p);
}

ErrorOrInt _execute(StringView cmd, ...)
{
    va_list  args;
    va_start(args, cmd);
    Process *p = process_vcreate(cmd, args);
    va_end(args);
    return process_execute(p);
}

ErrorOrStringView _execute_pipe(StringView input, StringView cmd, ...)
{
    va_list  args;
    va_start(args, cmd);
    Process *p = process_vcreate(cmd, args);
    va_end(args);
    TRY_TO(Int, StringView, process_background(p));
    TRY_TO(Size, StringView, write_pipe_write(&p->in, input));
    close(p->in.fd);
    process_wait(p);
    read_pipe_expect(&p->out);
    RETURN(StringView, p->out.buffer.view);
}
