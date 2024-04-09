/*
 * Copyright (c) 2024, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <stdio.h>

#include <errorcode.h>

#define ENOERROR 0
#define UNKNOWN ELAST

#define ERRORCODES(S)                                                                  \
    S(ENOERROR, "ENOERROR", "No error")                                                \
    S(EPERM, "EPERM", "Permission")                                                    \
    S(ENOENT, "ENOENT", "No such file or directory")                                   \
    S(ESRCH, "ESRCH", "No such process")                                               \
    S(EINTR, "EINTR", "Interrupted system call")                                       \
    S(EIO, "EIO", "Input/output error")                                                \
    S(ENXIO, "ENXIO", "Device not configured")                                         \
    S(E2BIG, "E2BIG", "Argument list too long")                                        \
    S(ENOEXEC, "ENOEXEC", "Exec format error")                                         \
    S(EBADF, "EBADF", "Bad file descriptor")                                           \
    S(ECHILD, "ECHILD", "No child processes")                                          \
    S(EDEADLK, "EDEADLK", "Resource deadlock avoided")                                 \
    S(ENOMEM, "ENOMEM", "Cannot allocate memory")                                      \
    S(EACCES, "EACCES", "Permission denied")                                           \
    S(EFAULT, "EFAULT", "Bad address")                                                 \
    S(ENOTBLK, "ENOTBLK", "Block device required")                                     \
    S(EBUSY, "EBUSY", "Device / Resource busy")                                        \
    S(EEXIST, "EEXIST", "File exists")                                                 \
    S(EXDEV, "EXDEV", "Cross-device link")                                             \
    S(ENODEV, "ENODEV", "Operation not supported by device")                           \
    S(ENOTDIR, "ENOTDIR", "Not a directory")                                           \
    S(EISDIR, "EISDIR", "Is a directory")                                              \
    S(EINVAL, "EINVAL", "Invalid argument")                                            \
    S(ENFILE, "ENFILE", "Too many open files in system")                               \
    S(EMFILE, "EMFILE", "Too many open files")                                         \
    S(ENOTTY, "ENOTTY", "Inappropriate ioctl for device")                              \
    S(ETXTBSY, "ETXTBSY", "Text file busy")                                            \
    S(EFBIG, "EFBIG", "File too large")                                                \
    S(ENOSPC, "ENOSPC", "No space left on device")                                     \
    S(ESPIPE, "ESPIPE", "Illegal seek")                                                \
    S(EROFS, "EROFS", "Read-only file system")                                         \
    S(EMLINK, "EMLINK", "Too many links")                                              \
    S(EPIPE, "EPIPE", "Broken pipe")                                                   \
    S(EDOM, "EDOM", "Numerical argument out of domain")                                \
    S(ERANGE, "ERANGE", "Result too large")                                            \
    S(EAGAIN, "EAGAIN", "Resource temporarily unavailable")                            \
    S(EINPROGRESS, "EINPROGRESS", "Operation now in progress")                         \
    S(EALREADY, "EALREADY", "Operation already in progress")                           \
    S(ENOTSOCK, "ENOTSOCK", "Socket operation on non-socket")                          \
    S(EDESTADDRREQ, "EDESTADDRREQ", "Destination address required")                    \
    S(EMSGSIZE, "EMSGSIZE", "Message too long")                                        \
    S(EPROTOTYPE, "EPROTOTYPE", "Protocol wrong type for socket")                      \
    S(ENOPROTOOPT, "ENOPROTOOPT", "Protocol not available")                            \
    S(EPROTONOSUPPORT, "EPROTONOSUPPORT", "Protocol not supported")                    \
    S(ESOCKTNOSUPPORT, "ESOCKTNOSUPPORT", "Socket type not supported")                 \
    S(ENOTSUP, "ENOTSUP", "Operation not supported")                                   \
    S(EPFNOSUPPORT, "EPFNOSUPPORT", "Protocol family not supported")                   \
    S(EAFNOSUPPORT, "EAFNOSUPPORT", "Address family not supported by protocol family") \
    S(EADDRINUSE, "EADDRINUSE", "Address already in use")                              \
    S(EADDRNOTAVAIL, "EADDRNOTAVAIL", "Can't assign requested address")                \
    S(ENETDOWN, "ENETDOWN", "Network is down")                                         \
    S(ENETUNREACH, "ENETUNREACH", "Network is unreachable")                            \
    S(ENETRESET, "ENETRESET", "Network dropped connection on reset")                   \
    S(ECONNABORTED, "ECONNABORTED", "Software caused connection abort")                \
    S(ECONNRESET, "ECONNRESET", "Connection reset by peer")                            \
    S(ENOBUFS, "ENOBUFS", "No buffer space available")                                 \
    S(EISCONN, "EISCONN", "Socket is already connected")                               \
    S(ENOTCONN, "ENOTCONN", "Socket is not connected")                                 \
    S(ESHUTDOWN, "ESHUTDOWN", "Can't send after socket shutdown")                      \
    S(ETOOMANYREFS, "ETOOMANYREFS", "Too many references: can't splice")               \
    S(ETIMEDOUT, "ETIMEDOUT", "Operation timed out")                                   \
    S(ECONNREFUSED, "ECONNREFUSED", "Connection refused")                              \
    S(ELOOP, "ELOOP", "Too many levels of symbolic links")                             \
    S(ENAMETOOLONG, "ENAMETOOLONG", "File name too long")                              \
    S(EHOSTDOWN, "EHOSTDOWN", "Host is down")                                          \
    S(EHOSTUNREACH, "EHOSTUNREACH", "No route to host")                                \
    S(ENOTEMPTY, "ENOTEMPTY", "Directory not empty")                                   \
    S(EPROCLIM, "EPROCLIM", "Too many processes")                                      \
    S(EUSERS, "EUSERS", "Too many users")                                              \
    S(EDQUOT, "EDQUOT", "Disc quota exceeded")                                         \
    S(ESTALE, "ESTALE", "Stale NFS file handle")                                       \
    S(EREMOTE, "EREMOTE", "Too many levels of remote in path")                         \
    S(EBADRPC, "EBADRPC", "RPC struct is bad")                                         \
    S(ERPCMISMATCH, "ERPCMISMATCH", "RPC version wrong")                               \
    S(EPROGUNAVAIL, "EPROGUNAVAIL", "RPC prog. not avail")                             \
    S(EPROGMISMATCH, "EPROGMISMATCH", "Program version wrong")                         \
    S(EPROCUNAVAIL, "EPROCUNAVAIL", "Bad procedure for program")                       \
    S(ENOLCK, "ENOLCK", "No locks available")                                          \
    S(ENOSYS, "ENOSYS", "Function not implemented")                                    \
    S(EFTYPE, "EFTYPE", "Inappropriate file type or format")                           \
    S(EAUTH, "EAUTH", "Authentication error")                                          \
    S(ENEEDAUTH, "ENEEDAUTH", "Need authenticator")                                    \
    S(EPWROFF, "EPWROFF", "Device power is off")                                       \
    S(EDEVERR, "EDEVERR", "Device error, e.g. paper out")                              \
    S(EOVERFLOW, "EOVERFLOW", "Value too large to be stored in data type")             \
    S(EBADEXEC, "EBADEXEC", "Bad executable")                                          \
    S(EBADARCH, "EBADARCH", "Bad CPU type in executable")                              \
    S(ESHLIBVERS, "ESHLIBVERS", "Shared library version mismatch")                     \
    S(EBADMACHO, "EBADMACHO", "Malformed Macho file")                                  \
    S(ECANCELED, "ECANCELED", "Operation canceled")                                    \
    S(EIDRM, "EIDRM", "Identifier removed")                                            \
    S(ENOMSG, "ENOMSG", "No message of desired type")                                  \
    S(EILSEQ, "EILSEQ", "Illegal byte sequence")                                       \
    S(ENOATTR, "ENOATTR", "Attribute not found")                                       \
    S(EBADMSG, "EBADMSG", "Bad message")                                               \
    S(EMULTIHOP, "EMULTIHOP", "Reserved")                                              \
    S(ENODATA, "ENODATA", "No message available on STREAM")                            \
    S(ENOLINK, "ENOLINK", "Reserved")                                                  \
    S(ENOSR, "ENOSR", "No STREAM resources")                                           \
    S(ENOSTR, "ENOSTR", "Not a STREAM")                                                \
    S(EPROTO, "EPROTO", "Protocol error")                                              \
    S(ETIME, "ETIME", "STREAM ioctl timeout")                                          \
    S(EOPNOTSUPP, "EOPNOTSUPP", "Operation not supported on socket")                   \
    S(ENOPOLICY, "ENOPOLICY", "No such policy registered")                             \
    S(ENOTRECOVERABLE, "ENOTRECOVERABLE", "State not recoverable")                     \
    S(EOWNERDEAD, "EOWNERDEAD", "Previous owner died")                                 \
    S(EQFULL, "EQFULL", "Interface output queue is full")

ErrorCode get_errorcode(int err)
{
    switch (err) {
#undef ERRORCODE
#define ERRORCODE(N, C, D) \
    case N:                \
        return (ErrorCode) { .errorno = err, .errorcode = C, .description = D };
        ERRORCODES(ERRORCODE)
#undef ERRORCODE
    default:
        return (ErrorCode) { .errorno = UNKNOWN, .errorcode = "UNKNOWN", .description = "Unknown error" };
    }
}

char const *errorcode_to_string(int err)
{
    static char buffer[1024];
    buffer[1023] = 0;
    ErrorCode error = get_errorcode(err);
    snprintf(buffer, 1023, "%s (%d): %s", error.errorcode, err, error.description);
    return buffer;
}
