#ifndef EXE_SLIB_H
#define EXE_SLIB_H

#include <stdint.h>
#include <stdbool.h>

void *memcpy(void *dst, const void *src, uint64_t n)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;

    for (uint64_t i = 0; i < n; i++)
        d[i] = s[i];

    return dst;
}
void *memset(void *dst, int c, uint64_t n)
{
    unsigned char *d = (unsigned char *)dst;
    unsigned char v = (unsigned char)c;

    for (uint64_t i = 0; i < n; i++)
        d[i] = v;

    return dst;
}
int memcmp(const void *a, const void *b, uint64_t n)
{
    const unsigned char *p = (const unsigned char *)a;
    const unsigned char *q = (const unsigned char *)b;

    for (uint64_t i = 0; i < n; i++)
    {
        if (p[i] != q[i])
            return (int)p[i] - (int)q[i];
    }

    return 0;
}
void ___chkstk_ms(void)
{
    /* stub: do nothing */
}
void erase_mem8(uint64_t map_addr, uint64_t len)
{
    // erase 1-bytes block * len
    uint8_t *map_int = (uint8_t *)map_addr;
    for (uint64_t i = 0; i < len; i++)
    {
        map_int[i] = 0;
    }
}
char *strconcat(char *dest, const char *s1, const char *s2)
{
    char *d = dest;

    // copy s1
    while (*s1)
    {
        *d++ = *s1++;
    }

    // copy s2
    while (*s2)
    {
        *d++ = *s2++;
    }

    // null terminate
    *d = '\0';
    return dest;
}
uint64_t div_ceil(uint64_t a, uint64_t b)
{
    if (a % b == 0)
        return a / b;
    else
        return a / b + 1;
}
uint64_t mem_cpy(void *dest, const void *src, uint64_t n)
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    for (uint64_t i = 0; i < n; i++)
        d[i] = s[i];

    return n;
}
// #define SYS_NUM_PE 777
// #define CALLSYS_ECHO 800
// #define CALLSYS_ALLOC_NEW_PAGE_AT_ADDR 1
// #define CALLSYS_GET_MAX_VIRT_ADDR_MAPPED_LOW 2
// #define CALLSYS_READ 1
// #define CALLSYS_RANDOM_INT 4
// #define CALLSYS_GET_PID 5
// #define CALLSYS_GET_TID 6
// #define CALLSYS_EXIT_PROCESS 7
// #define CALLSYS_CREATE_PROCESS 8
int64_t get_a_free_slot(uint64_t map_ptr, uint64_t start, uint64_t stop)
{
    uint64_t *map_int = (uint64_t *)map_ptr;
    for (uint64_t i = start; i <= stop; i++)
    {
        uint64_t tmp = (map_int[i]);
        if (tmp == 0)
            return i;
    }
    return -1;
}
int64_t get_a_free_slot_with_size(uint64_t map_addr, uint64_t start, uint64_t stop, uint64_t unit_byte_size)
{
    for (uint64_t i = start; i + unit_byte_size <= stop; i += unit_byte_size)
    {
        bool flag = false;
        for (uint64_t j = 0; j < unit_byte_size; j++)
        {
            uint8_t tmp = *((uint8_t *)(map_addr + i + j));
            if (tmp != 0)
            {
                flag = true;
                break;
            }
        }
        if (!flag)
            return i / unit_byte_size;
    }
    return -1;
}
int64_t callsys(int64_t sysnum, int64_t arg1, int64_t arg2, int64_t arg3, int64_t arg4, int64_t arg5, int64_t arg6)
{
    int64_t ret;
    // uint64_t sysnum = SYS_NUM_PE;
    asm volatile(
        "mov %1, %%rax\n"
        "mov %2, %%rdi\n"
        "mov %3, %%rsi\n"
        "mov %4, %%rdx\n"
        "mov %5, %%r10\n"
        "mov %6, %%r8\n"
        "mov %7, %%r9\n"
        "syscall\n"
        "mov %%rax, %0\n"
        : "=r"(ret)
        : "r"(sysnum), "r"(arg1), "r"(arg2), "r"(arg3), "r"(arg4), "r"(arg5), "r"(arg6)
        : "%rax", "%rdi", "%rsi", "%rdx", "%r10", "%r8", "%r9", "memory");
    return ret;
}
uint64_t echo(char *str)
{
    uint64_t len = 0;
    while (str[len])
        len++;

    return callsys(1, 1, (uint64_t)str, len, 0, 0, 0);
}
int64_t read_file(const char *path, uint8_t *buf, uint64_t offset, uint64_t len)
{
    int64_t fd;
    int64_t ret;

    // open(path, O_RDONLY, 0)
    fd = callsys(257, -100, (uint64_t)path, 0, 0, 0, 0);

    if ((int64_t)fd < 0)
        return fd;

    // read(fd, buf, len, offset)
    ret = callsys(17, fd, (uint64_t)buf, len, offset, 0, 0);

    // close(fd)
    callsys(3, fd, 0, 0, 0, 0, 0);

    return ret; 
}
#define SYS_mmap 9
#define PROT_RWE (1 | 2 | 4) // PROT_READ | PROT_WRITE | PROT_EXEC
#define MAP_ANON 0x20
#define MAP_PRIV 0x02
#define MAP_FIXED_NOREPLACE 0x100000
uint64_t extend_cr3(uint64_t addr, uint64_t size)
{
    uint64_t ret = callsys(SYS_mmap, addr, size, PROT_RWE, MAP_PRIV | MAP_ANON | MAP_FIXED_NOREPLACE, (uint64_t)-1, 0);

    if ((int64_t)ret < 0)
        return 0; // alloc fail

    return ret;
}
uint64_t alloc1(uint64_t addr)
{
    uint64_t ret = callsys(SYS_mmap, addr, 4096, PROT_RWE, MAP_PRIV | MAP_ANON | MAP_FIXED_NOREPLACE, (uint64_t)-1, 0);

    if ((int64_t)ret < 0)
        return 0; // alloc fail

    return ret;
}
void erase_block(uint64_t *ptr)
{
    for (uint64_t i = 0; i < 4096 / 8; i++)
        ((uint64_t *)ptr)[i] = 0;
}
#define SYS_getpid 39
#define SYS_gettid 186
static inline int64_t get_pid(void)
{
    return callsys(SYS_getpid, 0, 0, 0, 0, 0, 0);
}

static inline int64_t get_tid(void)
{
    return callsys(SYS_gettid, 0, 0, 0, 0, 0, 0);
};
int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return (unsigned char)(*s1) == (unsigned char)(*s2);
}
uint64_t strlen(const char *s)
{
    const char *p = s;
    while (*p)
    {
        p++;
    }
    return (uint64_t)(p - s);
}
void set_gsbase(uint64_t addr)
{
    callsys(158, 0x1001, addr, 0, 0, 0, 0);
}
int strcmp_i(const char *s1, const char *s2)
{
    while (*s1 && *s2)
    {
        char c1 = *s1;
        char c2 = *s2;

        
        if (c1 >= 'A' && c1 <= 'Z')
            c1 += ('a' - 'A');
        if (c2 >= 'A' && c2 <= 'Z')
            c2 += ('a' - 'A');

        if (c1 != c2)
            return 0;

        s1++;
        s2++;
    }

    
    return strlen(s1) == strlen(s2);
}
int strcmp_iw(const uint16_t *s1, const uint16_t *s2)
{
    while (*s1 && *s2)
    {
        uint16_t c1 = *s1;
        uint16_t c2 = *s2;

        // ASCII uppercase → lowercase
        if (c1 >= 'A' && c1 <= 'Z')
            c1 += ('a' - 'A');
        if (c2 >= 'A' && c2 <= 'Z')
            c2 += ('a' - 'A');

        if (c1 != c2)
            return 0;

        s1++;
        s2++;
    }

    
    return (*s1 == 0 && *s2 == 0);
}
void echoi(int64_t x)
{
    // echo integer in hexadecimal;
    char buf[32];
    for (int i = 0; i < 16; i++)
    {
        uint8_t nibble = (x >> ((15 - i) * 4)) & 0xF;
        if (nibble < 10)
            buf[i] = '0' + nibble;
        else
            buf[i] = 'a' + (nibble - 10);
    }
    buf[16] = '\0';
    echo(buf);
}
void echow(char *in)
{
    uint64_t i = 0;
    while (in[i])
    {
        callsys(1, 1, (uint64_t)in + i, 1, 0, 0, 0);
        i += 2;
    }
    echo("\n");
}
uint64_t strlen_w16(uint16_t *in)
{
    uint64_t i = 0;
    while (in[i])
    {
        i += 1;
    }
    return i;
}
#endif // { EXE_NTDLL_SLIB_H }
