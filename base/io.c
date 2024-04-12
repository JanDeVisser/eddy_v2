/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stddef.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include <base/errorcode.h>
#include <base/io.h>

#define BUF_SZ 65536
#define NO_SOCKET ((socket_t) - 1)

typedef struct {
    int           fd;
    StringBuilder buffer;
} Socket;

DA_WITH_NAME(Socket, Sockets);
DA_IMPL(Socket);

Sockets s_sockets = { 0 };

ErrorOrInt socket_fd(socket_t socket)
{
    if (socket < s_sockets.size && s_sockets.elements[socket].fd >= 0) {
        RETURN(Int, s_sockets.elements[socket].fd);
    }
    ERROR(Int, IOError, 0, "socket cannot be mapped to file descriptor");
}

socket_t socket_allocate(int fd)
{
    for (size_t ix = 0; ix < s_sockets.size; ++ix) {
        if (s_sockets.elements[ix].fd == -1) {
            Socket *s = s_sockets.elements + ix;
            s->fd = fd;
            s->buffer.length = 0;
            return ix;
        }
    }
    da_append_Socket(&s_sockets, (Socket) { .fd = fd });
    return s_sockets.size - 1;
}

void socket_close(socket_t socket)
{
    close(s_sockets.elements[socket].fd);
    s_sockets.elements[socket].fd = -1;
}

ErrorOrInt fd_make_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        ERROR(Int, IOError, 0, "Cannot get file descriptor flags: %s", strerror(errno));
    }
    flags = flags | O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0) {
        ERROR(Int, IOError, 0, "Cannot make file descriptor non-blocking: %s", strerror(errno));
    }
    RETURN(Int, fd);
}

ErrorOrSockAddrIn tcpip_address_resolve(StringView ip_address)
{
    struct addrinfo hints, *res, *res0;
    int             error;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    char buf[ip_address.length + 1];
    if ((error = getaddrinfo(sv_cstr(ip_address, buf), NULL, &hints, &res0)) != 0) {
        ERROR(SockAddrIn, IOError, 0, "Error resolving IP address '%.*s': %s",
            SV_ARG(ip_address), gai_strerror(error));
    }
    if (!res0) {
        ERROR(SockAddrIn, IOError, 0, "Could not resolve IP address '%.*s': %s",
            SV_ARG(ip_address), gai_strerror(error));
    }
    struct sockaddr_in addr;
    for (res = res0; res; res = res->ai_next) {
        if (res->ai_family == AF_INET && res->ai_socktype == SOCK_STREAM) {
            assert(res->ai_addrlen == sizeof(struct sockaddr_in));
            memcpy(&addr, res->ai_addr, sizeof(struct sockaddr_in));
            freeaddrinfo(res0);
            RETURN(SockAddrIn, addr);
        }
    }
    freeaddrinfo(res0);
    ERROR(SockAddrIn, IOError, 0, "Could not resolve address '%.*s' to an IP address", SV_ARG(ip_address));
}

ErrorOrSocket unix_socket_listen(StringView socket_name)
{
    int const listen_fd = socket(PF_LOCAL, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        ERROR(Socket, IOError, 0, "Cannot create socket: %s", strerror(errno));
    }

    struct sockaddr_un sock_addr = { 0 };
    sock_addr.sun_family = AF_LOCAL;
    if (socket_name.length >= sizeof(sock_addr.sun_path)) {
        ERROR(Socket, IOError, 0, "Local socket name '%.*s' too long: %zu >= %zu",
            SV_ARG(socket_name), socket_name.length, sizeof(sock_addr.sun_path));
    }
    memcpy(sock_addr.sun_path, socket_name.ptr, socket_name.length);
    sock_addr.sun_path[socket_name.length] = '\0';
    size_t serv_size = offsetof(struct sockaddr_un, sun_path) + socket_name.length + 1;
    if (bind(listen_fd, (struct sockaddr *) &sock_addr, serv_size) < 0) {
        ERROR(Socket, IOError, 0, "Cannot bind to local socket '%.*s': %s", SV_ARG(socket_name), strerror(errno));
    }
    if (listen(listen_fd, 1) < 0) {
        ERROR(Socket, IOError, 0, "Cannot listen on local socket '%.*s': %s", SV_ARG(socket_name), strerror(errno));
    }
    RETURN(Socket, socket_allocate(listen_fd));
}

ErrorOrSocket tcpip_socket_listen(StringView ip_address, int port)
{
    NYI();
}

ErrorOrSocket socket_accept(socket_t socket)
{
    int conn_fd = accept(s_sockets.elements[socket].fd, NULL, NULL);
    if (conn_fd < 0) {
        ERROR(Socket, IOError, 0, "Cannot accept connection on local socket: %s", strerror(errno));
    }
    TRY_TO(Int, Socket, fd_make_nonblocking(conn_fd));
    RETURN(Socket, socket_allocate(conn_fd));
}

ErrorOrSocket unix_socket_connect(StringView socket_name)
{
    int conn_fd = socket(PF_LOCAL, SOCK_STREAM, 0);
    if (conn_fd < 0) {
        ERROR(Socket, IOError, 0, "Cannot create socket: %s", strerror(errno));
    }

    struct sockaddr_un sock_addr = { 0 };
    sock_addr.sun_family = AF_LOCAL;
    if (socket_name.length >= sizeof(sock_addr.sun_path)) {
        ERROR(Socket, IOError, 0, "Local socket name '%.*s' too long: %zu >= %zu",
            SV_ARG(socket_name), socket_name.length, sizeof(sock_addr.sun_path));
    }
    memcpy(sock_addr.sun_path, socket_name.ptr, socket_name.length);
    sock_addr.sun_path[socket_name.length] = '\0';
    size_t sock_addr_size = offsetof(struct sockaddr_un, sun_path) + socket_name.length + 1;
    if (connect(conn_fd, (struct sockaddr *) &sock_addr, sock_addr_size) < 0) {
        ERROR(Socket, IOError, 0, "Cannot connect to local socket '%.*s': %s", SV_ARG(socket_name), strerror(errno));
    }
    TRY_TO(Int, Socket, fd_make_nonblocking(conn_fd));
    RETURN(Socket, socket_allocate(conn_fd));
}

ErrorOrSocket tcpip_socket_connect(StringView ip_address, int port)
{
    int conn_fd;
    if ((conn_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        ERROR(Socket, IOError, 0, "Cannot create socket: %s", strerror(errno));
    }

    struct sockaddr_in server_address = TRY_TO(SockAddrIn, Socket, tcpip_address_resolve(ip_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);

    if (connect(conn_fd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        ERROR(Socket, IOError, 0, "Cannot connect to TCP/IP address '%.*s:%d': %s",
            SV_ARG(ip_address), port, strerror(errno));
    }
    TRY_TO(Int, Socket, fd_make_nonblocking(conn_fd));
    RETURN(Socket, socket_allocate(conn_fd));
}

ErrorOrSize read_available_bytes(Socket *s)
{
    char   buffer[BUF_SZ];
    size_t total = 0;
    while (true) {
        ssize_t bytes_read = read(s->fd, buffer, BUF_SZ);
        if (bytes_read < 0) {
            if (errno == EAGAIN) {
                break;
            }
            if (errno != EINTR) {
                ERROR(Size, HttpError, 0, "Failed to read from socket: %s", errorcode_to_string(errno));
            }
            continue;
        }
        if (bytes_read == 0) {
            break;
        }
        total += bytes_read;
        sb_append_chars(&s->buffer, buffer, bytes_read);
        if (bytes_read < BUF_SZ) {
            break;
        }
    }
    RETURN(Size, total);
}

ErrorOrSize socket_fill_buffer(Socket *s)
{
    TRY(Size, read_available_bytes(s));
    if (s->buffer.length > 0) {
        RETURN(Size, s->buffer.length);
    }

    struct pollfd poll_fd = { 0 };
    poll_fd.fd = s->fd;
    poll_fd.events = POLLIN;

    while (true) {
        if (poll(&poll_fd, 1, -1) == -1) {
            if (errno == EINTR) {
                continue;
            }
            ERROR(Size, IOError, 0, "Error polling socket connection: %s", errorcode_to_string(errno));
        }
        if (poll_fd.revents & POLLIN) {
            break;
        }
        if (poll_fd.revents & POLLHUP) {
            ERROR(Size, IOError, -1, "Socket connection closed");
        }
    }
    TRY(Size, read_available_bytes(s));
    assert(s->buffer.length > 0);
    RETURN(Size, s->buffer.length);
}

ErrorOrStringView socket_read(socket_t socket, size_t count)
{
    Socket       *s = s_sockets.elements + socket;
    StringBuilder out = { 0 };

    trace(IPC, "socket_read(%zu)", count);
    do {
        TRY_TO(Size, StringView, socket_fill_buffer(s));
        if (!count && (s->buffer.length > 0)) {
            trace(IPC, "socket_read(%zu) => NULL", count);
            RETURN(StringView, sv_null());
        }
        if (s->buffer.length <= count) {
            count -= s->buffer.length;
            if (out.length == 0) {
                out = s->buffer;
                s->buffer = (StringBuilder) { 0 };
            } else {
                sb_append_sv(&out, s->buffer.view);
                s->buffer.length = 0;
            }
        } else {
            if (out.length == 0) {
                out = s->buffer;
                s->buffer = sb_copy_chars(out.ptr + count, out.length - count);
                out.length = count;
            } else {
                sb_append_chars(&out, s->buffer.view.ptr, count);
                memmove((char *) s->buffer.ptr, s->buffer.ptr + count, count);
                s->buffer.length -= count;
            }
            count = 0;
        }
    } while (count > 0);
    trace(IPC, "socket_read(%zu) => %zu", count, out.view.length);
    RETURN(StringView, out.view);
}

ErrorOrStringView socket_readln(socket_t socket)
{
    Socket       *s = s_sockets.elements + socket;
    StringBuilder out = sb_create();
    while (true) {
        TRY_TO(Size, StringView, socket_fill_buffer(s));
        trace(IPC, "socket_readln: %zu bytes available", s->buffer.length);
        for (size_t ix = 0; ix < s->buffer.length; ++ix) {
            char ch = s->buffer.ptr[ix];
            switch (ch) {
            case '\r':
                break;
            case '\n':
                if (ix < s->buffer.length - 1) {
                    memmove((char *) s->buffer.ptr, s->buffer.ptr + ix + 1, s->buffer.length - ix + 1);
                }
                s->buffer.length -= ix + 1;
                trace(IPC, "socket_readln: %zu bytes consumed", ix + 1);
                RETURN(StringView, out.view);
            default:
                sb_append_char(&out, ch);
                break;
            }
        }
        trace(IPC, "socket_readln: buffer depleted");
        s->buffer.length = 0;
    }
}

ErrorOrSize socket_write(socket_t socket, char const *buffer, size_t num)
{
    Socket *s = s_sockets.elements + socket;
    ssize_t total = 0;
    trace(IPC, "socket_write(%zu)", num);
    while (total < num) {
        ssize_t written = write(s->fd, buffer, num - total);
        if (written < 0) {
            if (errno == EAGAIN) {
                trace(IPC, "socket_write(%zu) - EAGAIN (retrying)", num);
                continue;
            }
            trace(IPC, "socket_write(%zu) - error %s", num, errorcode_to_string(errno));
            ERROR(Size, IOError, 0, "Error writing to socket: %s", errorcode_to_string(errno));
        }
        if (written == 0) {
            trace(IPC, "socket_write(%zu) - incomplete write", num);
            ERROR(Size, IOError, 0, "Incomplete write to socket: %d < %d", written, num);
        }
        trace(IPC, "socket_write: chunk %zu", written);
        total += written;
    }
    trace(IPC, "socket_write: %zu", total);
    RETURN(Size, total);
}

ErrorOrSize socket_writeln(socket_t socket, StringView sv)
{
    TRY(Size, socket_write(socket, sv.ptr, sv.length));
    char eol = '\n';
    TRY(Size, socket_write(socket, &eol, 1));
    RETURN(Size, sv.length + 1);
}

ErrorOrStringView read_file_by_name(StringView file_name)
{
    int fd = open(sv_cstr(file_name, NULL), O_RDONLY);
    if (fd < 0) {
        ERROR(StringView, IOError, errno, "Could not open file");
    }
    ErrorOrStringView ret = read_file(fd);
    close(fd);
    return ret;
}

ErrorOrStringView read_file_at(int dir_fd, StringView file_name)
{
    char buf[file_name.length + 1];
    int  fd = openat(dir_fd, sv_cstr(file_name, buf), O_RDONLY);
    if (fd < 0) {
        ERROR(StringView, IOError, errno, "Could not open file");
    }
    ErrorOrStringView ret = read_file(fd);
    close(fd);
    return ret;
}

ErrorOrStringView read_file(int fd)
{
    struct stat sb;
    if (fstat(fd, &sb) < 0) {
        ERROR(StringView, IOError, errno, "Could not fstat file");
    }
    size_t sz = sb.st_size;
    return sv_read(fd, sz);
}

ErrorOrSize write_file_by_name(StringView file_name, StringView contents)
{
    char buf[file_name.length + 1];
    int  fd = open(sv_cstr(file_name, buf), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        ERROR(Size, IOError, errno, "Could not open file");
    }
    ErrorOrSize ret = write_file(fd, contents);
    close(fd);
    return ret;
}

ErrorOrSize write_file_at(int dir_fd, StringView file_name, StringView contents)
{
    char buf[file_name.length + 1];
    int  fd = openat(dir_fd, sv_cstr(file_name, buf), O_RDONLY);
    if (fd < 0) {
        ERROR(Size, IOError, errno, "Could not open file");
    }
    ErrorOrSize ret = write_file(fd, contents);
    close(fd);
    return ret;
}

ErrorOrSize write_file(int fd, StringView contents)
{
    size_t total = 0;
    while (total < contents.length) {
        ssize_t ret = write(fd, contents.ptr + total, contents.length - total);
        if (ret < 0) {
            ERROR(Size, IOError, errno, "Could not write to file");
        }
        total += ret;
    }
    RETURN(Size, total);
}
