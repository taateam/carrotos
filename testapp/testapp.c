// testapp.c - minimal ELF with no libc

typedef unsigned long long uint64_t;

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

// --- add_char ---

void add_char(char *dest, char c){
    static int idx[64] = {0};  
    static int cur = 0;         
    if(c == '\0'){
        dest[idx[cur]] = '\0';
        cur++;
        return;
    }
    dest[idx[cur]++] = c;
}

// --- string_to_num ---
long long string_to_num(char *str){
    long long res = 0;
    int neg = 0;
    if(*str == '-'){
        neg = 1;
        str++;
    }
    while(*str){
        res = res*10 + (*str - '0');
        str++;
    }
    return neg ? -res : res;
}

long long powerx(int a, int b)
{
    long long tmp = 1;
    for (int i = 0; i < b; i++)
        tmp *= a;
    return tmp;
}
// --- num_to_str ---
void num_to_str1(long long num, char *buf)
{
    int start_pos = 0;
    if (num < 0)
    {
        buf[0] = '-';
        start_pos = 1;
        num = 0 - num;
    }
    int power = 0;
    while (1)
    {
        if (num / powerx(10, power) == 0)
            break;
        power++;
    }
    power -- ;
    int i = 0;
    for (; power >= 0; power--)
    {
        int tmp = num / powerx(10, power);
        num %= powerx(10, power);
        char c = tmp + 48;
        buf[start_pos + i] = c;
        i++;
    }
    buf[i + 1] = '\0';
}

void str_2_output_buffer(char *in, char *out)
{
    int i = 0;
    for (; i < 64; i++)
    {
        if (in[i] == '\0')
            break;
        out[i * 2] = 0;
        out[i * 2 + 1] = in[i];
    }
    out[i * 2 + 2] = 0;
    out[i * 2 + 3] = 0;
}
void _start() {
     char buf[128];

    
    int fd = syscall(2,                   // sys_open
                     (unsigned long)"/dev/tty1",
                     0,                   // O_RDONLY
                     0,0,0,0);

    if (fd != 0) {
        syscall(33, fd, 0,0,0,0,0);       // dup2(fd, 0)
        syscall(3, fd,0,0,0,0,0);         // close(fd)
    }

    
    fd = syscall(2,
                 (unsigned long)"/dev/tty1",
                 1,                   // O_WRONLY
                 0,0,0,0);

    if (fd != 1) {
        syscall(33, fd, 1,0,0,0,0);       // dup2(fd, 1)
        syscall(3, fd,0,0,0,0,0);
    }

    
    while (1) {
        long n = syscall(0,          // sys_read
            0,                       // fd = stdin = tty1
            (unsigned long)buf,
            128,
            0,0,0
        );

        // split by_spaces
        char * null_str = "";
        char arr [64] [7];
        int j = 0;
        for (unsigned int i =0; i<128;i++){
			if(i % 2 == 0)
				continue;
			if(buf[i] == 0){
				add_char(arr[j],'\0');
				break;
			}else if(buf[i] == ' '){
				add_char(arr[j],'\0');
				j++;
			}else{
				add_char(arr[j],buf[i]);
			}
		}
		
		// convert to long long ;
		long long args [7];
		for (j = 0; j<7;j++){
			args[j] = string_to_num(arr[j]);
		}
		long long rs = syscall(args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
		char rs_str[64];
		num_to_str1(rs, rs_str);
		char rs_str1[128];
		str_2_output_buffer(rs_str, rs_str1);
		
		
        syscall(1,
            1,                       // fd = stdout = tty1
            (unsigned long)rs_str1,
            80,
            0,0,0
        );
    }

    
    syscall(60, 0,0,0,0,0,0);
}

