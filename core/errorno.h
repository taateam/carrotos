#ifndef ERRORNO_H
#define ERRORNO_H

#define EINVAL 22 /* Invalid argument */
#define EISDIR 21 /* Is a directory */
#define ENOENT 2  /* No such file or directory */
#define ENOTDIR 20 /* Not a directory */
#define EIO 5     /* I/O error */
#define EFAULT 14 /* Bad address */
#define EPERM 1   // Operation not permitted
#define ESRCH 3   // No such process
#define EAGAIN 11 // Try again / Resource temporarily unavailable
#define ECHILD 10
#define EACCES 13 //Permission denied
#define EEXIST 17 // file exist but try to create
#define ENODEV 19// no such device
#define EINVAL 22 // Invalid argument
#define ENOTTY  25  // Not a typewriter (not a TTY)

#define EBADF 9     // "Bad file descriptor"
#define ENOTSOCK 88 /* Socket operation on non-socket */
#define EPROTONOSUPPORT 93
#define ESOCKTNOSUPPORT 94
#define EOPNOTSUPP 95
#define EAFNOSUPPORT 97
#define EADDRINUSE 98
#define ENETUNREACH 101
#define ECONNRESET 104
#define ENOBUFS 105
#define EISCONN 106 // Linux
#define ENOTCONN 107
#define ETIMEDOUT 110
#define ECONNREFUSED 111
#define EINPROGRESS 115
#define ENOEXEC 114

#endif // ERRORNO_H
