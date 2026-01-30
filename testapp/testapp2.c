// testapp.c - minimal ELF with no libc

typedef unsigned long long uint64_t;
typedef unsigned int       uint32_t;
typedef unsigned short     uint16_t;
typedef unsigned char      uint8_t;
typedef long               int64_t;

/* ================= socket constants ================= */

#define AF_UNIX      1
#define SOCK_STREAM  1
#define SOCK_DGRAM   2

/* ================= syscall numbers (x86_64 Linux) ================= */

#define SYS_listen   50
#define SYS_connect    42
#define SYS_accept   43
#define SYS_read      0
#define SYS_write     1
#define SYS_close     3
#define SYS_socket    41
#define SYS_bind      49
#define SYS_sendto    44
#define SYS_recvfrom  45
#define SYS_unlink    87
#define SYS_exit      60

/* ================= sockaddr_un ================= */

struct sockaddr_un {
    uint16_t sun_family;     // AF_UNIX
    char     sun_path[108];  // pathname
};

unsigned long syscall(unsigned long num,
                       unsigned long a1,
                       unsigned long a2,
                       unsigned long a3,
                       unsigned long a4,
                       unsigned long a5,
                       unsigned long a6)
{
    unsigned long ret;

    register unsigned long rax asm("rax") = num;
    register unsigned long rdi asm("rdi") = a1;
    register unsigned long rsi asm("rsi") = a2;
    register unsigned long rdx asm("rdx") = a3;
    register unsigned long r10 asm("r10") = a4;
    register unsigned long r8  asm("r8")  = a5;
    register unsigned long r9  asm("r9")  = a6;

    asm volatile (
        "syscall"
        : "=a" (ret)
        : "a" (rax), "r" (rdi), "r" (rsi), "r" (rdx), "r" (r10), "r" (r8), "r" (r9)
        : "rcx", "r11", "memory"
    );

    return ret;
}
void print(char* str){
	syscall(800, (uint64_t)str, 0, 0, 0, 0, 0);   // debug syscall;
}

void sleep(uint64_t s){
	unsigned long long ts[2];
	ts[0]=s; 
	ts[1]=0; 
	syscall(35, (uint64_t)&ts, 0, 0, 0, 0, 0); 
}
/* ================= entry ================= */

void _start(void)
{
    int sock;
    struct sockaddr_un srv;
    char buf[128];

    sleep(1); 

    print("[client] start\n");

    sock = syscall(SYS_socket, AF_UNIX, SOCK_STREAM, 0, 0, 0, 0);
    if (sock < 0) while (1);

    srv.sun_family = AF_UNIX;
    for (int i = 0; i < 108; i++) srv.sun_path[i] = 0;
    char path[] = "/tmp/a.sock";
    for (int i = 0; path[i]; i++) srv.sun_path[i] = path[i];

    uint32_t addr_len = 2 + sizeof("/tmp/a.sock");

    if (syscall(SYS_connect, sock, (uint64_t)&srv, addr_len, 0, 0, 0) < 0) {
        print("[client] connect failed\n");
        while (1);
    }

    print("[client] connected\n");

    sleep(1);
    char msg[] = "hello via send/recv";

    /* -------- send() -------- */
    syscall(
        SYS_sendto,
        sock,
        (uint64_t)msg,
        sizeof(msg) - 1,
        0,          // flags
        0,          // addr = NULL (STREAM)
        0
    );

    /* -------- recv() -------- */
    long r = syscall(
        SYS_recvfrom,
        sock,
        (uint64_t)buf,
        sizeof(buf) - 1,
        0,          // flags
        0,          // src addr ignored
        0
    );

    if (r > 0) {
        buf[r] = 0;
        print("[client] recv: ");
        print(buf);
        print("\n");
    }

    // syscall(SYS_close, sock, 0, 0, 0, 0, 0);
    print("[client] done\n");

    while (1) sleep(1000);
}
