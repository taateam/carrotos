#ifndef _WIN_ERRNO_H_
#define _WIN_ERRNO_H_

/* =========================================================
 *  Basic types
 * ========================================================= */

typedef unsigned long NTSTATUS;
typedef unsigned long DWORD;
typedef int BOOL;

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

/* =========================================================
 *  NTSTATUS helpers
 * ========================================================= */

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

/* Severity */
#define STATUS_SEVERITY_SUCCESS 0x0
#define STATUS_SEVERITY_INFORMATIONAL 0x1
#define STATUS_SEVERITY_WARNING 0x2
#define STATUS_SEVERITY_ERROR 0x3

/* =========================================================
 *  Common NTSTATUS values
 * ========================================================= */

#define STATUS_SUCCESS ((NTSTATUS)0x00000000)
#define STATUS_UNSUCCESSFUL ((NTSTATUS)0xC0000001)
#define STATUS_NOT_IMPLEMENTED ((NTSTATUS)0xC0000002)
#define STATUS_INVALID_PARAMETER ((NTSTATUS)0xC000000D)
#define STATUS_NO_MEMORY ((NTSTATUS)0xC0000017)
#define STATUS_ACCESS_VIOLATION ((NTSTATUS)0xC0000005)
#define STATUS_ILLEGAL_INSTRUCTION ((NTSTATUS)0xC000001D)
#define STATUS_INTEGER_DIVIDE_BY_ZERO ((NTSTATUS)0xC0000094)
#define STATUS_BREAKPOINT ((NTSTATUS)0x80000003)
#define STATUS_STACK_OVERFLOW ((NTSTATUS)0xC00000FD)
#define STATUS_PRIVILEGED_INSTRUCTION ((NTSTATUS)0xC0000096)
#define STATUS_DATATYPE_MISALIGNMENT ((NTSTATUS)0x80000002)
#define STATUS_INVALID_HANDLE ((NTSTATUS)0xC0000008)
#define STATUS_OBJECT_NAME_NOT_FOUND ((NTSTATUS)0xC0000034)
#define STATUS_OBJECT_PATH_NOT_FOUND ((NTSTATUS)0xC000003A)
#define STATUS_OBJECT_NAME_COLLISION ((NTSTATUS)0xC0000035)
#define STATUS_BUFFER_TOO_SMALL ((NTSTATUS)0xC0000023)
#define STATUS_END_OF_FILE ((NTSTATUS)0xC0000011)
#define STATUS_TIMEOUT ((NTSTATUS)0x00000102)

/* =========================================================
 *  Win32 ERROR_ codes
 * ========================================================= */

#define ERROR_SUCCESS 0
#define ERROR_INVALID_FUNCTION 1
#define ERROR_FILE_NOT_FOUND 2
#define ERROR_PATH_NOT_FOUND 3
#define ERROR_TOO_MANY_OPEN_FILES 4
#define ERROR_ACCESS_DENIED 5
#define ERROR_INVALID_HANDLE 6
#define ERROR_NOT_ENOUGH_MEMORY 8
#define ERROR_INVALID_DATA 13
#define ERROR_OUTOFMEMORY 14
#define ERROR_INVALID_DRIVE 15
#define ERROR_NO_MORE_FILES 18
#define ERROR_WRITE_PROTECT 19
#define ERROR_BAD_UNIT 20
#define ERROR_NOT_READY 21
#define ERROR_BAD_COMMAND 22
#define ERROR_CRC 23
#define ERROR_BAD_LENGTH 24
#define ERROR_SEEK 25
#define ERROR_WRITE_FAULT 29
#define ERROR_READ_FAULT 30
#define ERROR_GEN_FAILURE 31
#define ERROR_SHARING_VIOLATION 32
#define ERROR_LOCK_VIOLATION 33
#define ERROR_HANDLE_EOF 38
#define ERROR_HANDLE_DISK_FULL 39
#define ERROR_NOT_SUPPORTED 50
#define ERROR_FILE_EXISTS 80
#define ERROR_CANNOT_MAKE 82
#define ERROR_INVALID_PARAMETER 87
#define ERROR_BROKEN_PIPE 109
#define ERROR_DISK_FULL 112
#define ERROR_CALL_NOT_IMPLEMENTED 120
#define ERROR_INSUFFICIENT_BUFFER 122
#define ERROR_INVALID_NAME 123
#define ERROR_NEGATIVE_SEEK 131
#define ERROR_DIR_NOT_EMPTY 145
#define ERROR_ALREADY_EXISTS 183
#define ERROR_ENVVAR_NOT_FOUND 203
#define ERROR_FILENAME_EXCED_RANGE 206
#define ERROR_DIRECTORY 267
#define ERROR_NOT_OWNER 288
#define STATUS_DISK_FULL   ((NTSTATUS)0xC000007F)

/* =========================================================
 *  errno (Windows-style)
 * ========================================================= */

#define EPERM 1
#define ENOENT 2
#define ESRCH 3
#define EINTR 4
#define EIO 5
#define ENXIO 6
#define E2BIG 7
#define ENOEXEC 8
#define EBADF 9
#define ECHILD 10
#define EAGAIN 11
#define ENOMEM 12
#define EACCES 13
#define EFAULT 14
#define EBUSY 16
#define EEXIST 17
#define EXDEV 18
#define ENODEV 19
#define ENOTDIR 20
#define EISDIR 21
#define EINVAL 22
#define ENFILE 23
#define EMFILE 24
#define ENOTTY 25
#define EFBIG 27
#define ENOSPC 28
#define ESPIPE 29
#define EROFS 30
#define EMLINK 31
#define EPIPE 32
#define EDOM 33
#define ERANGE 34
#define ENOSYS 38

/* =========================================================
 *  NTSTATUS → Win32 ERROR
 * ========================================================= */

static inline DWORD ntstatus_to_winerror(NTSTATUS st)
{
    switch (st)
    {
    case STATUS_SUCCESS:
        return ERROR_SUCCESS;
    case STATUS_ACCESS_VIOLATION:
        return ERROR_ACCESS_DENIED;
    case STATUS_INVALID_HANDLE:
        return ERROR_INVALID_HANDLE;
    case STATUS_OBJECT_NAME_NOT_FOUND:
        return ERROR_FILE_NOT_FOUND;
    case STATUS_OBJECT_PATH_NOT_FOUND:
        return ERROR_PATH_NOT_FOUND;
    case STATUS_OBJECT_NAME_COLLISION:
        return ERROR_FILE_EXISTS;
    case STATUS_NO_MEMORY:
        return ERROR_OUTOFMEMORY;
    case STATUS_BUFFER_TOO_SMALL:
        return ERROR_INSUFFICIENT_BUFFER;
    case STATUS_NOT_IMPLEMENTED:
        return ERROR_CALL_NOT_IMPLEMENTED;
    case STATUS_INVALID_PARAMETER:
        return ERROR_INVALID_PARAMETER;
    case STATUS_DISK_FULL:
        return ERROR_DISK_FULL;
    default:
        return ERROR_GEN_FAILURE;
    }
}

/* =========================================================
 *  NTSTATUS → errno
 * ========================================================= */

static inline int ntstatus_to_errno(NTSTATUS st)
{
    switch (st)
    {
    case STATUS_SUCCESS:
        return 0;
    case STATUS_ACCESS_VIOLATION:
        return EFAULT;
    case STATUS_INVALID_HANDLE:
        return EBADF;
    case STATUS_OBJECT_NAME_NOT_FOUND:
        return ENOENT;
    case STATUS_OBJECT_PATH_NOT_FOUND:
        return ENOENT;
    case STATUS_OBJECT_NAME_COLLISION:
        return EEXIST;
    case STATUS_NO_MEMORY:
        return ENOMEM;
    case STATUS_BUFFER_TOO_SMALL:
        return ERANGE;
    case STATUS_NOT_IMPLEMENTED:
        return ENOSYS;
    case STATUS_INVALID_PARAMETER:
        return EINVAL;
    case STATUS_DISK_FULL:
        return ENOSPC;
    default:
        return EIO;
    }
}

#endif /* _WIN_ERRNO_H_ */
       //;