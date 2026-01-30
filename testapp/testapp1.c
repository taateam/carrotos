// test_proc.c - minimal ELF, no libc, test /proc

typedef unsigned long long u64;
typedef long long i64;

/* raw syscall */
static inline u64 syscall(u64 n,
    u64 a1, u64 a2, u64 a3,
    u64 a4, u64 a5, u64 a6)
{
    u64 ret;
    register u64 rax asm("rax") = n;
    register u64 rdi asm("rdi") = a1;
    register u64 rsi asm("rsi") = a2;
    register u64 rdx asm("rdx") = a3;
    register u64 r10 asm("r10") = a4;
    register u64 r8  asm("r8")  = a5;
    register u64 r9  asm("r9")  = a6;

    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(rax), "r"(rdi), "r"(rsi),
          "r"(rdx), "r"(r10), "r"(r8), "r"(r9)
        : "rcx", "r11", "memory"
    );
    return ret;
}

/* helpers */
static inline void write_str(char *s)
{
    u64 len = 0;
    while (s[len]) len++;
    syscall(1, 1, (u64)s, len, 0,0,0);
}

static inline void write_buf(char *buf, u64 n)
{
    syscall(1, 1, (u64)buf, n, 0,0,0);
}

/* linux_dirent64 */
struct linux_dirent64 {
    u64 d_ino;
    i64 d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[];
};

void _start()
{
    char buf[4096];
    i64 fd, n;

    /* ===== TEST 1: /proc/self/maps ===== */
    //~ write_str("=== /proc/self/maps ===\n");
    //~ fd = syscall(2, (u64)"/proc/3/maps", 0, 0,0,0,0);
    //~ while ((n = syscall(0, fd, (u64)buf, sizeof(buf),0,0,0)) > 0)
        //~ write_buf(buf, n);
    //~ syscall(3, fd, 0,0,0,0,0);

    /* ===== TEST 2: /proc/self/status ===== */
    write_str("\n=== /proc/self/status ===\n");
    fd = syscall(2, (u64)"/proc/3/status", 0, 0,0,0,0);
    while ((n = syscall(0, fd, (u64)buf, sizeof(buf),0,0,0)) > 0)
        write_buf(buf, n);
    syscall(3, fd, 0,0,0,0,0);

    /* ===== TEST 3: getdents64(/proc) ===== */
    write_str("\n=== /proc entries ===\n");
    fd = syscall(2, (u64)"/proc", 0, 0,0,0,0);

    while ((n = syscall(217, fd, (u64)buf, sizeof(buf),0,0,0)) > 0) {
        u64 off = 0;
        while (off < n) {
            struct linux_dirent64 *d =
                (struct linux_dirent64 *)(buf + off);
            write_str(d->d_name);
            write_str("\n");
            off += d->d_reclen;
        }
    }
    syscall(3, fd, 0,0,0,0,0);

    while (1);
    //syscall(60, 0,0,0,0,0,0);
}
