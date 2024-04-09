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

#include <errorcode.h>
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

    pthread_t thread;
    int       ret;
    pipe->current = (StringView) { 0 };
    if ((ret = pthread_create(&thread, NULL, (void *(*) (void *) ) read_pipe_read, (void *) pipe)) != 0) {
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

#define DRAIN_SIZE 64 * 1024

void read_pipe_drain(ReadPipe *pipe)
{
    char buffer[DRAIN_SIZE];
    condition_acquire(pipe->condition);
    while (true) {
        ssize_t count = read(pipe->fd, buffer, sizeof(buffer) - 1);
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
            break;
        }
        if (count == 0) {
            break;
        }
        if (errno == EINTR) {
            continue;
        }
        fatal("read_pipe_drain(): Error reading child process output: %s", errorcode_to_string(errno));
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
    ssize_t count;
    while (true) {
        count = write(pipe->fd, buf, num);
        if (count >= 0) {
            break;
        }
        if (errno != EINTR) {
            ERROR(Size, ProcessError, errno, "Error writing to child process input");
        }
    }
    RETURN(Size, count);
}

static void process_dump(Process *p)
{
    trace(LIB, "Command: '%.*s' #arguments: %zu", SV_ARG(p->command), p->arguments.size);
    for (size_t ix = 0; ix < p->arguments.size; ++ix) {
        trace(LIB, "Arg #%zu: '%.*s'", ix, SV_ARG(p->arguments.strings[ix]));
    }
}

Process *process_create_sl(StringView cmd, StringList *args)
{
    Process *p = MALLOC(Process);
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

void sigchld(int)
{
    trace(PROCESS, "SIGCHLD caught");
}

ErrorOrInt process_start(Process *p)
{
    signal(SIGCHLD, sigchld);
    size_t sz = p->arguments.size;
    size_t bufsz = p->command.length + 1;
    for (size_t ix = 0u; ix < sz; ++ix) {
        bufsz += p->arguments.strings[ix].length + 1;
    }
    char  buf[bufsz];
    char *argv[sz + 2];
    argv[0] = (char *) sv_cstr(p->command, buf);
    char *bufptr = buf + p->command.length + 1;
    for (size_t ix = 0u; ix < sz; ++ix) {
        argv[ix + 1] = (char *) sv_cstr(p->arguments.strings[ix], bufptr);
        bufptr = bufptr + p->arguments.strings[ix].length + 1;
    }
    argv[sz + 1] = NULL;
    StringView args = sl_join(&p->arguments, sv_from(" "));
    trace(PROCESS, "[CMD] %.*s %.*s", SV_ARG(p->command), SV_ARG(args));
    sv_free(args);

    // signal(SIGCHLD, SIG_IGN);
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
        if (sv_empty(p->stdout_file)) {
            read_pipe_connect_child(&p->out, STDOUT_FILENO);
        } else {
            char stdout_file_buffer[p->stdout_file.length + 1];
            int  fd = open(sv_cstr(p->stdout_file, stdout_file_buffer), O_WRONLY | O_CREAT | O_TRUNC, 0777);
            assert_msg(fd, "Could not open stdout stream '%.*s' for '%.*s': %s",
                SV_ARG(p->stdout_file), SV_ARG(p->command), strerror(errno));
            while (dup2(fd, STDOUT_FILENO) == -1 && (errno == EINTR)) { }
        }
        if (sv_empty(p->stderr_file)) {
            read_pipe_connect_child(&p->err, STDERR_FILENO);
        } else {
            char stderr_file_buffer[p->stderr_file.length + 1];
            int  fd = open(sv_cstr(p->stderr_file, stderr_file_buffer), O_WRONLY | O_CREAT | O_TRUNC, 0777);
            assert_msg(fd, "Couldn not open stderr stream '%.*s' for '%.*s': %s",
                SV_ARG(p->stderr_file), SV_ARG(p->command), strerror(errno));
            while (dup2(fd, STDERR_FILENO) == -1 && (errno == EINTR)) { }
        }
        execvp(argv[0], argv);
        fatal("execvp(%.*s) failed: %s", SV_ARG(p->command), errorcode_to_string(errno));
    }
    write_pipe_connect_parent(&p->in);
    read_pipe_connect_parent(&p->out);
    read_pipe_connect_parent(&p->err);
    if (!sv_is_cstr(p->command)) {
        free(argv[0]);
    }
    for (size_t ix = 0; ix < sz; ++ix) {
        if (!sv_is_cstr(p->arguments.strings[ix])) {
            free(argv[ix + 1]);
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
    va_list args;
    va_start(args, cmd);
    Process *p = process_vcreate(cmd, args);
    va_end(args);
    return process_execute(p);
}

ErrorOrStringView _execute_pipe(StringView input, StringView cmd, ...)
{
    va_list args;
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
