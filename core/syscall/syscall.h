#ifndef SYSCALL_H
#define SYSCALL_H

/*
#define SYSCALL_PRINT 1
#define SYSCALL_HLT 2
*/

void sys_echo(uint64_t str_addr)
{
    echo((char *)str_addr);
}
//===========================================================================
#include "syscall1.h"
#include "syscall2.h"
#include "syscall3.h"
#include "syscall4.h"
//===========================================================================

uint64_t syscall6(uint64_t num,
                  uint64_t arg0, uint64_t arg1, uint64_t arg2,
                  uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
    uint64_t ret;
    __asm__ volatile(
        "mov %1, %%rax\n"
        "mov %2, %%rdi\n"
        "mov %3, %%rsi\n"
        "mov %4, %%rdx\n"
        "mov %5, %%r10\n"
        "mov %6, %%r8\n"
        "mov %7, %%r9\n"
        "int $0x80\n"
        "mov %%rax, %0\n"
        : "=r"(ret)
        : "r"(num), "r"(arg0), "r"(arg1), "r"(arg2),
          "r"(arg3), "r"(arg4), "r"(arg5)
        : "rax", "rdi", "rsi", "rdx", "r10", "r8", "r9", "memory");
    return ret;
}
typedef uint64_t (*syscall_func_t)(
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5,
    uint64_t arg6);

// syscall table
static syscall_func_t syscall_table[] = {
    // --- File system ---
    [0] = (syscall_func_t)sys_read,    // read(fd, buf, count)
    [1] = (syscall_func_t)sys_write,   // write(fd, buf, count)
    [2] = (syscall_func_t)sys_open,    // open(path, flags, mode)
    [3] = (syscall_func_t)sys_close,   // close(fd)
    [4] = (syscall_func_t)sys_stat,    // stat(path, stat_buf)
    [7] = (syscall_func_t)sys_poll,    // poll (fake ready)
    [8] = (syscall_func_t)sys_seek,    // lseek(fd, offset, whence)
    [10] = (syscall_func_t)sys_reboot,   // fsync(fd)
    [82] = (syscall_func_t)sys_rename, // rename(oldpath, newpath)
    [83] = (syscall_func_t)sys_mkdir,  // mkdir(path, mode)
    [87] = (syscall_func_t)sys_unlink, // unlink(path)
    [231] = (syscall_func_t)sys_mount, // mount

    // pipe
    [22] = (syscall_func_t)sys_pipe,   // rt_sigreturn();
    [293] = (syscall_func_t)sys_pipe2, // rt_sigreturn();
    // --- Memory management ---
    [9] = (syscall_func_t)sys_mmap,    // mmap(addr, length, prot, flags, fd, offset)
    [11] = (syscall_func_t)sys_munmap, // munmap(addr, length)
    [12] = (syscall_func_t)sys_brk,    // brk(addr)

    // time

    [201] = (syscall_func_t)sys_time,
    [96] = (syscall_func_t)sys_gettimeofday,
    [228] = (syscall_func_t)sys_clock_gettime,
    [227] = (syscall_func_t)sys_clock_settime,
    [229] = (syscall_func_t)sys_clock_getres,
    [124] = (syscall_func_t)sys_adjtimex,
    [35] = (syscall_func_t)sys_sleep,

    // random
    [318] = (syscall_func_t)sys_getrandom,

    // --- Signal ---
    [13] = (syscall_func_t)sys_rt_sigaction,   // rt_sigaction(signum, act, oldact, sigsetsize)
    [14] = (syscall_func_t)sigprocmask,        // sigprocmask(how, set, oldset)
    [15] = (syscall_func_t)sys_rt_sigreturn,   // rt_sigreturn();
    [314] = (syscall_func_t)rt_sigqueueinfo,   // rt_sigqueueinfo(pid, sig, info)
    [315] = (syscall_func_t)rt_tgsigqueueinfo, // rt_tgsigqueueinfo(pid, tid, sig, info)

    // --- Process / Thread ---
    [39] = (syscall_func_t)sys_getpid,      // getpid()
    [56] = (syscall_func_t)sys_clone,       // clone(flags, child_stack, ptid, ctid, newtls)
    [57] = (syscall_func_t)sys_fork,        // fork()
    [59] = (syscall_func_t)sys_execve,      // execve(path, argv, envp)
    [60] = (syscall_func_t)sys_exit,        // exit(status)
    [61] = (syscall_func_t)sys_waitpid,     // ✅ waitpid(pid, status, options)
    [62] = (syscall_func_t)sys_kill,        // kill(pid, sig)
    [109] = (syscall_func_t)sys_setpgid,    // setpgid(pid, pgid)
    [110] = (syscall_func_t)sys_getppid,    // getppid()
    [112] = (syscall_func_t)sys_setsid,     // setsid()
    [121] = (syscall_func_t)sys_getpgid,    // ✅ getpgid(pid)
    [124] = (syscall_func_t)sys_getsid,     // ✅ getsid(pid)
    [231] = (syscall_func_t)sys_exit_group, // exit_group(status)
    [234] = (syscall_func_t)sys_tgkill,     // tgkill(tgid, tid, sig)

    // ---Futex ---;
    [202] = (syscall_func_t)sys_futex,
    // --- Terminal control ---
    [215] = (syscall_func_t)sys_tcsetpgrp, // ✅ tcsetpgrp(fd, pgrp)

    // --- Network ---
    [41] = (syscall_func_t)sys_socket,      // socket(domain, type, protocol)
    [42] = (syscall_func_t)sys_connect,     // connect(sockfd, addr, addrlen)
    [43] = (syscall_func_t)sys_accept,      // accept(sockfd, addr, addrlen)
    [44] = (syscall_func_t)sys_sendto,      // sendto(sockfd, buf, len, flags, dest_addr, addrlen)
    [45] = (syscall_func_t)sys_recvfrom,    // recvfrom(sockfd, buf, len, flags, src_addr, addrlen)
    [48] = (syscall_func_t)sys_shutdown,    // shutdown(sockfd, how)
    [49] = (syscall_func_t)sys_bind,        // bind(sockfd, addr, addrlen)
    [50] = (syscall_func_t)sys_listen,      // listen(sockfd, backlog)
    [51] = (syscall_func_t)sys_getsockname, // getsockname(sockfd, addr, addrlen)
    [52] = (syscall_func_t)sys_getpeername, // getpeername(sockfd, addr, addrlen)
    [54] = (syscall_func_t)sys_setsockopt,  // setsockopt(sockfd, level, optname, optval, optlen)

    // --- User / Credential ---
    [102] = (syscall_func_t)sys_getuid,
    [104] = (syscall_func_t)sys_getgid,
    [105] = (syscall_func_t)sys_setuid,
    [106] = (syscall_func_t)sys_setgid,
    [107] = (syscall_func_t)sys_geteuid,
    [108] = (syscall_func_t)sys_getegid,
    [113] = (syscall_func_t)sys_setreuid,
    [114] = (syscall_func_t)sys_setregid,
    [117] = (syscall_func_t)sys_setresuid,
    [119] = (syscall_func_t)sys_setresgid,
    
    // commented / not implemented
    // [48] = sys_socketpair
    // [49] = sys_send
    // [50] = sys_recv
    // [55] = sys_getsockopt
    [776] = (syscall_func_t)sys_get_main_tid,
    // [777] = (syscall_func_t)sys_create_process_winpe,
    [800] = (syscall_func_t)sys_echo,
};
uint64_t syscall_dispatch(uint64_t arg1, uint64_t arg2, uint64_t arg3,
                          uint64_t arg4, uint64_t arg5, uint64_t arg6)
{
    uint64_t num = 0;
    asm("mov %%r10, %0" : "=r"(num));
    uint64_t n_syscalls = sizeof(syscall_table) / sizeof(syscall_func_t);

    if (num >= n_syscalls || syscall_table[num] == NULL)
        return (uint64_t)-1;

    echo("sys ");
    echoi(num);
    echo(": ");
    uint64_t return_v = 0;
    if (num == 57)
    {
        echo("\n");
        return_v = sys_fork();
        return return_v;
    }
    if (num == 56)
    {
        echo("\n");
        return_v = sys_clone(arg1, (void *)arg2, (int *)arg3, (int *)arg4, (uint64_t)arg5);
        return return_v;
    }
    return_v = syscall_table[num](arg1, arg2, arg3, arg4, arg5, arg6);
    asm volatile(
        ""
        :
        : /* no inputs */
        : "rbx", "rcx", "rdx", "rsi", "rdi",
          "r8", "r9", "r10", "r11",
          "r12", "r13", "r14", "r15",
          "memory");
    // uint64_t y = 4;
    // y += 3;
    echoi(return_v);
    echo("\n");
    return return_v;
}

#endif // SYSCALL_H
