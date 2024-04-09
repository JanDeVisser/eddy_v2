/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __IO_H__
#define __IO_H__

#include <netinet/in.h>

#include <base/error_or.h>
#include <base/sv.h>

typedef size_t socket_t;
ERROR_OR_ALIAS(Socket, socket_t);

ERROR_OR_ALIAS(SockAddrIn, struct sockaddr_in);

ErrorOrSockAddrIn tcpip_address_resolve(StringView ip_address);
ErrorOrInt        fd_make_nonblocking(int fd);
ErrorOrSocket     unix_socket_listen(StringView socket_name);
ErrorOrSocket     tcpip_socket_listen(StringView ip_address, int port);
ErrorOrSocket     socket_accept(socket_t socket);
ErrorOrSocket     unix_socket_connect(StringView socket_name);
ErrorOrSocket     tcpip_socket_connect(StringView ip_address, int port);
ErrorOrStringView socket_read(socket_t socket, size_t count);
ErrorOrStringView socket_readln(socket_t socket);
ErrorOrSize       socket_write(socket_t socket, char const *buffer, size_t num);
ErrorOrSize       socket_writeln(socket_t socket, StringView sv);
void              socket_close(socket_t socket);
ErrorOrStringView read_file_by_name(StringView file_name);
ErrorOrStringView read_file_at(int dir_fd, StringView file_name);
ErrorOrStringView read_file(int fd);
ErrorOrSize       write_file_by_name(StringView file_name, StringView contents);
ErrorOrSize       write_file_at(int dir_fd, StringView file_name, StringView contents);
ErrorOrSize       write_file(int fd, StringView contents);

#endif /* __IO_H__ */
