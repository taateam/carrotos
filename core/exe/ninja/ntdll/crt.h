// #include "libs.h"
// #include "common.h"
// #include "tls.h"

uint64_t __security_cookie; // /GS stack protector
uint64_t __security_cookie_complement;
int _fmode = 0; // default text mode
int _commode = 0;
int _fltused = 0; // required MSVC float support
#define HEAP_START 0x100000000ULL
uint64_t __carrotos_heap_top = HEAP_START;

uint64_t get_main_entry_pointaddr()
{
    uint64_t peb_addr = 0;
    asm volatile("mov %%gs:0x60, %0" : "=r"(peb_addr));
    struct PEB *peb = (struct PEB *)peb_addr;
    uint64_t main_base_addr = get_base_address_from_peb(0);
    struct pe_optional_header64 *main_opt_header64 = get_opt_header64(main_base_addr);
    return main_opt_header64->address_of_entry + main_base_addr;
}
uint64_t count_argv(uint16_t *argv)
{
    uint64_t count = 0;
    bool last_char_is_white_space = false;
    uint64_t len = strlen_w16(argv);
    for (uint64_t i = 0; i < len; i++)
    {
        if (argv[i] == L' ' || argv[i] == L'\t' || argv[i] == L'\n')
        {
            last_char_is_white_space = true;
        }
        else
        {
            if (!last_char_is_white_space)
            {
                count++;
            }
            last_char_is_white_space = false;
        }
    }
    return count;
}
void split(uint16_t *argv_str, uint16_t *argv_buf, uint16_t **argv_ptr_buf, uint64_t argc)
{
    for (uint64_t i = 0; i < strlen_w16(argv_str); i++)
    {
        if (argv_str[i] == L' ' || argv_str[i] == L'\t' || argv_str[i] == L'\n')
        {
            argv_buf[i] = L'\0';
        }
        else
        {
            argv_buf[i] = argv_str[i];
        }
    }
    uint64_t arg_index = 0;
    argv_ptr_buf[0] = (char *)&argv_buf[0];
    bool last_char_isnull = false;
    for (uint64_t i = 1; i < strlen((char *)argv_buf); i++)
    {
        if (argv_buf[i] == '\0')
        {
            last_char_isnull = true;
        }
        else
        {
            if (last_char_isnull)
            {
                arg_index++;
                argv_ptr_buf[arg_index] = (char *)&argv_buf[i];
            }
            last_char_isnull = false;
        }
    }
    argv_ptr_buf[arg_index] = NULL;
}
static inline uint64_t rdtsc(void)
{
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}
void init_global_vars()
{
    uint64_t cookie = 0;

    cookie ^= rdtsc();
    cookie ^= (uint64_t)get_pid() << 32;
    cookie ^= (uint64_t)callsys(SYS_gettid, 0, 0, 0, 0, 0, 0) << 16;
    cookie ^= (uint64_t)&cookie;
    cookie ^= (uint64_t)0x4222;

    
    if (cookie == 0 || cookie == 0x2B992DDFA232ULL)
        cookie ^= 0xBB40E64EULL;

    __security_cookie = cookie;
    __security_cookie_complement = ~cookie;
    ;
}
void init_heap()
{
    uint64_t peb_addr = 0;
    asm volatile("mov %%gs:0x60, %0" : "=r"(peb_addr));
    struct PEB *peb = (struct PEB *)peb_addr;
    uint64_t pid = get_pid();
    extend_cr3(__carrotos_heap_top, 4096 * 4);
    peb->ProcessHeap = __carrotos_heap_top;
    __carrotos_heap_top -= 4096 * 4;
}
void exit_process(int ret)
{
    callsys(60, ret, 0, 0, 0, 0, 0);
};


// void run_crt_xcu(void)
// {
//     for (void (**fn)() = __xc_a; fn < __xc_z; fn++)
//     {
//         if (*fn)
//             (*fn)();
//     }
// }
//==================================================================================
int __getmainargs(int *argc, char ***argv, char ***envp, int expand_wildcards, int *new_mode)
{
    echo(" getmainargs");
    return 0;
}
int64_t malloc(uint64_t size)
{
    echo(" malloc:");
    uint64_t block_count = (size + 4095) / 4096;
    extend_cr3(__carrotos_heap_top, block_count * 4096);
    uint64_t allocated_addr = __carrotos_heap_top;
    echoi(allocated_addr);
    __carrotos_heap_top += block_count * 4096;
    return allocated_addr;
}
void free(){
    echo(" free");
}
uint64_t _onexit(void *x)
{
    echo(" _onexit");
    return 1;
}
uint64_t exit(void *x)
{
    echo(" exit");
    exit_process(0);
    return 1;
}
bool WriteConsoleA(void *h, const void *buf, uint32_t len, uint32_t *written, void *reserved)
{
    echo ("\n");
    echo((char *)buf);
    echo ("\n");
    return true;
}
//==================================================================================
void test()
{
    uint64_t base_main = get_base_address_from_peb(0);
    struct pe_optional_header64 *opt_header64 = get_opt_header64(base_main);
    struct data_directory *import_main = &opt_header64->data_directories[IMAGE_DIRECTORY_ENTRY_IMPORT];
    struct pe_image_import_descriptor *iat_entry = (struct pe_image_import_descriptor *)(import_main->virtual_address + base_main);
    uint32_t all_im_rva = 0, all_im_rva1 = 0;
    while (1)
    {
        if (!iat_entry->name)
            continue;
        char *dll_name = (char *)(iat_entry->name + base_main);
        if (memcmp(dll_name, "msvcrt.dll", 10) == 0)
        {
            all_im_rva = (uint64_t)(iat_entry->original_first_thunk);
            all_im_rva1 = (uint64_t)(iat_entry->first_thunk);
            break;
        }
        iat_entry++;
    }
    uint64_t *imp_rva_arr = all_im_rva + base_main;
    uint64_t *imp_rva_arr1 = all_im_rva1 + base_main;
    uint64_t i = 0;
    while (1)
    {
        uint64_t imp_rva = imp_rva_arr[i];
        uint64_t imp_rva1 = imp_rva_arr1[i];
        if (imp_rva == 0)
            break;
        struct image_import_by_name *im = (struct image_import_by_name *)(imp_rva + base_main);
        struct image_import_by_name *im1 = (struct image_import_by_name *)(imp_rva1);
        char *func_name = &im->name;
        echo(func_name);
        echo(" : ");
        echoi(im1);
        echo("\n");
        i++;
    };
}
void crt_init_c()
{
    // argv;
    struct PEB *peb = NULL;
    asm volatile("mov %%gs:0x60, %0" : "=r"(peb));
    uint16_t *argv = (uint16_t *)peb->process_params_buf.CommandLine.Buffer;
    uint64_t argv_total_len = strlen_w16(argv);
    uint64_t argc = count_argv(argv);
    uint8_t argv_buf[argv_total_len * 2 + 2];
    uint16_t *argv_ptr_buf[argc + 1];
    split(argv, argv_buf, argv_ptr_buf, argc);
    // envp;
    uint16_t *envp = (uint16_t *)peb->process_params_buf.Environment;
    uint64_t envp_total_len = strlen_w16(envp);
    uint64_t envc = count_argv(envp);
    uint8_t envp_buf[envp_total_len * 2 + 2];
    uint16_t *envp_ptr_buf[envc + 1];
    split(envp, envp_buf, envp_ptr_buf, envc);
    //==================
    init_global_vars();
    init_heap();
    // run_crt_xcu();
    uint64_t peb_addr = (uint64_t)peb;
    uint64_t entry_point = get_main_entry_pointaddr();
    uint64_t new_stack_top = 0x400000000000ULL - 4096;
    extend_cr3(new_stack_top - 40960, 40960);
    // extend_cr3(0x180000000 , 40960*1024);
    uint64_t *lll = 0x1418a509b - 512 * 8;
    // for (uint64_t i = 0; i < 1024; i++)
    //     lll[i] = 0x140;
    test();
    asm volatile(
        // switch stack
        "mov %0, %%rsp\n"

        // fake return address
        "sub $8, %%rsp\n"
        "movq $0, (%%rsp)\n"

        // shadow space + alignment
        "sub $0x200, %%rsp\n"

        // Windows x64 args
        "mov %1, %%rcx\n"
        "xor %%rdx, %%rdx\n"
        "xor %%r8,  %%r8\n"
        "xor %%r9,  %%r9\n"

        // jump into PE entry
        "jmp *%2\n"
        :
        : "r"(new_stack_top), "r"(peb_addr), "r"(entry_point)
        : "memory", "rcx", "rdx", "r8", "r9");
    __builtin_unreachable();
    // int ret = ((int (*)(int))entry_point)((uint64_t)peb);
    // exit_process(ret);
};