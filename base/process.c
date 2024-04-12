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
