#ifndef CORE_H
#define CORE_H

#include <stdint.h>
#include <stdbool.h>
extern uint64_t processes_cr3_ptr[];
extern uint64_t current_tick;
extern uint8_t *memory_map;
extern void *stack64_bottom_aps;
extern const uint64_t MAX_CORES_COUNT;
extern const uint64_t MAX_THREADS_COUNT;
extern const uint64_t MAX_PROCESSES_COUNT;
extern const uint64_t KERNEL_STACK_SIZE;
extern const void *end_kernel;
extern const void *page_table_l4;
extern const void *page_table_l4_b;

#define MAX_FD_INNER 853 // 20 blocks / sizeof(struct file_inner) 1024

#define HM 0x400000000000ULL
#define AP_STACK64_SIZE 4 * 4096
#define q(ptr) ((typeof(ptr))((uint64_t)(ptr) + 0x200000000000ULL))
#define PTE_PRESENT (1ULL << 0)
#define PTE_RW (1ULL << 1)
#define PTE_USER (1ULL << 2)
#define PTE_NX (1ULL << 63)

#define PROC_MAX_SIG_HANDLER 64
#define PROC_MAX_SM 64
#define MAX_CMD_LINE_LEN 512
#define MAX_ENV_LEN 512
#define MAX_PATH 4 * 256

uint64_t threads_info_lock1;
uint64_t threads_info_lock2;
typedef enum __attribute__((mode(DI)))
{
    PROCESS_NOT_EXIST,
    PROCESS_RUNNING,
    PROCESS_IDLE,
    PROCESS_STOPPED,
    PROCESS_ENDED_BUT_NOT_CLEANED,
    PROCESS_HANDLING_SIGNAL
} process_state_t;
struct shared_mem;
typedef struct
{
    uint64_t pid;
    // uint64_t cr3;
    process_state_t state;
    uint64_t main_thread_id;
    uint64_t signal;
    uint64_t threads_count;
    uint64_t pgid;
    uint64_t sid;
    // uint64_t *internal_threads_id;
    uint64_t fd_addr;
    uint64_t parent_pid;
    uint64_t heap_start_addr;
    uint64_t heap_end_addr;
    uint64_t sig_handlers[PROC_MAX_SIG_HANDLER];
    uint64_t uid;
    uint64_t gid;
    uint64_t euid;
    uint64_t egid;
    uint64_t reuid;
    uint64_t regid;
    uint64_t resuid;
    uint64_t resgid;
    uint64_t lock;
    uint64_t return_code;
    struct shared_mem *sm[PROC_MAX_SM];
    char cmd_line[MAX_CMD_LINE_LEN];
    char env[MAX_ENV_LEN];
    char cwd[MAX_PATH];
} process_info_t;

typedef struct
{
    uint64_t apic_id;
    uint64_t current_thread;            // offset 8
    uint64_t current_thread_kernel_rsp; // offset 16
    uint64_t current_thread_user_rsp;   // offset 24
    uint64_t core_rsp;
} core_info_t;
typedef struct
{
    // 1. General-purpose registers
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rdi, rsi, rbp;
    uint64_t rbx, rdx, rcx, rax;

    // 2. 5 registers
    uint64_t rip, cs, rflags, rsp, ss, gs24;

    // 4. SIMD/FPU context (align 16 for fxsave)
    uint8_t xsave_area[832] __attribute__((aligned(64))); // offset 192 bytes;

    
    uint64_t fs_base, gs_base;
} user_thread_context;
typedef enum __attribute__((mode(DI)))
{
    THREAD_NOT_EXIST,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED_KEY,
    THREAD_BLOCKED_MOUSE,
    THREAD_BLOCKED_IO,
    THREAD_BLOCKED_NETWORK,
    THREAD_BLOCKED_UDS,
    THREAD_BLOCKED_PID,
    THREAD_BLOCKED_FUTEX,
    THREAD_BLOCKED_OTHER,
    THREAD_IDLE,
    THREAD_DEAD,
    THREAD_ENDED_BUT_NOT_CLEANED,
    THREAD_WAITING_SIGNAL
} thread_state_t;
typedef enum __attribute__((mode(DI)))
{
    CONTEXT_TYPE_ALU,
    CONTEXT_TYPE_SSE
} context_type_t;
typedef struct
{
    uint64_t pid; // process id
    uint64_t tid; // thread id
    thread_state_t state;
    struct timeval wakeup_time;
    uint64_t stack_addr;
    uint64_t kernel_stack_addr;
    user_thread_context context;
    uint64_t ptid;
    uint8_t signal;
    uint64_t signal_mask;
    // uint64_t sig_handlers[64];
    uint64_t sleep_flag_addr;
    context_type_t context_type;
    struct timeval lock_time;
    uint64_t return_code;
} thread_info_t;
// thread_info_t all_user_threads[2560];

typedef struct
{
    unsigned char e_ident[16]; // ELF magic + info
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry; // <<< Entry point
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf64_Ehdr;

enum file_type
{
    FILE_NONE,
    FILE_REGULAR,
    FILE_CHAR_DEV,
    FILE_PIPE,
    FILE_DIR,
    FILE_SOCKET,
    FILE_SOCKET_UDS,
    FILE_TTY,
    FILE_FRAME_BUFFER,
    FILE_SPECIAL_DIR,
    FILE_SPECIAL 
};

enum open_flags
{
    O_RDONLY = 0x1,
    O_WRONLY = 0x2,
    O_RDWR = 0x3,
    O_CREAT = 0x4,
    O_APPEND = 0x8,
    O_TRUNC = 0x10,
    
};

struct file_metadata
{
    uint64_t owner;
    uint64_t group;
    uint64_t mode; 
    uint64_t size;
    uint64_t created;
    uint64_t modified;
};

struct file_inner
{
    uint64_t global_id;
    
    struct file_metadata meta;
    uint64_t offset; 
    int open_flags;  
    enum file_type type;
    uint64_t res_id;
    // struct file_ops ops;
    
};
extern core_info_t cores_info[];
extern process_info_t *processes_info_ptr[];
extern thread_info_t *threads_info_ptr[];
uint32_t read_apic_id()
{
    uint32_t apic_id;
    __asm__ volatile(
        "movl $0xFEE00020, %%eax\n\t"
        "movl (%%eax), %%eax\n\t"
        "shrl $24, %%eax\n\t"
        "movl %%eax, %0"
        : "=r"(apic_id)
        :
        : "eax");
    return apic_id;
}

void set_gs_base()
{
    uint32_t apic_id = read_apic_id();
    core_info_t *core_ptr = &cores_info[apic_id];

    core_ptr->apic_id = apic_id;
    core_ptr->core_rsp = ((uint64_t)&stack64_bottom_aps) + (apic_id + 1) * AP_STACK64_SIZE;
    uint64_t base = (uint64_t)core_ptr;
    uint32_t low = (uint32_t)(base & 0xFFFFFFFF);
    uint32_t high = (uint32_t)(base >> 32);

    __asm__ volatile(
        "movl $0xC0000101, %%ecx\n\t" // IA32_GS_BASE
        "movl %0, %%eax\n\t"
        "movl %1, %%edx\n\t"
        "wrmsr"
        :
        : "r"(low), "r"(high)
        : "rax", "rdx", "ecx");
}

uint64_t get_apic_id()
{
    uint64_t apic_id;
    __asm__ volatile(
        "mov %%gs:0, %0"
        : "=r"(apic_id));
    return apic_id;
}

uint64_t get_current_thread_id()
{
    uint64_t ptr;
    __asm__ volatile(
        "mov %%gs:8, %0"
        : "=r"(ptr));
    return ptr;
}
uint64_t get_current_pid()
{
    uint64_t tid = get_current_thread_id();
    thread_info_t **tmp = threads_info_ptr;
    uint64_t pid = tmp[tid]->pid;
    // process_info_t *tmp1 = (process_info_t *)&processes_info_ptr;
    return pid;
}
extern void tick_interrupt_handler();
extern void tick_interrupt_handler1();
#include "interrupt.h"
extern void dummy_interrupt_handler();
extern void dummy_interrupt_handler1();
extern void syscall_interrupt_handler();
extern void halt();
extern void invalid_opcode_handler();
extern void seg_fault_handler();
extern void float_error_handler();
// extern void *idt_table;
extern uint64_t start_flag;
extern uint64_t core0_prepared;
void core_main() // entry from 64bit asm for each core
{
    // void *tmp = &idt_table;
    set_gs_base();
    // for (uint64_t i = 0; i < 8; i++)
    //     register_interrupt_handler(i, dummy_interrupt_handler, 0, 0x8E);
    // register_interrupt_handler(8, dummy_interrupt_handler1, 0, 0x8E);
    // for (uint64_t i = 10; i < 33; i++)
    //     register_interrupt_handler(i, dummy_interrupt_handler1, 0, 0x8E);
    // for (uint64_t i = 34; i < 256; i++)
    //     register_interrupt_handler(i, dummy_interrupt_handler, 0, 0x8E);
    // register_interrupt_handler(8, dummy_interrupt_handler1, 0, 0x8E);
    while (start_flag == 0)
    {
        ;
    }
    asm volatile("sti");
    // asm volatile("ud2");
    for (;;)
    {
        1 + 1;
        // asm volatile("ud2");
    };
}

bool is_memory_block_used(uint64_t index)
{
    return (memory_map[index]) % 2 == 1;
}
void set_memory_block_used_when_boot(uint64_t index, bool used)
{
    uint8_t *tmp = (uint8_t *)&memory_map;
    if (used)
        tmp[index] = 1; // set byte = 1
    else
        tmp[index] = 0; // clear byte = 0
}
uint64_t div_ceil(uint64_t var_1, uint64_t var_2)
{
    uint64_t result = var_1 / var_2;
    if (var_1 % var_2 > 0)
        result++;
    return result;
}
void mark_reserved_blocks_as_used()
{
    uint64_t end_kernel_addr = (uint64_t)&end_kernel;
    uint64_t total_kernel_mem_blocks_count = div_ceil(end_kernel_addr - HM, 4096);
    ;
    uint64_t total_blocks_of_memory_map = div_ceil(total_memory_blocks_count, 4096);
    for (uint64_t i = 0; i < total_kernel_mem_blocks_count + total_blocks_of_memory_map; i++)
        set_memory_block_used_when_boot(i, true);
}
void panic(char input[])
{
    clear_screen();
    echo("Kernel panic: ");
    echo(input);
    while (true)
        __asm__ volatile("hlt");
}
uint64_t get_a_free_block()
{
    // return addr under 128T
    uint64_t total_blocks = total_memory_blocks_count;
    uint64_t total_kernel_mem_blocks_count = (((uint64_t)&end_kernel) - HM) / 4096;

    
    uint8_t *tmp = (uint8_t *)&memory_map;
    for (uint64_t i = total_kernel_mem_blocks_count; i < total_blocks; i++)
    {
        if (tmp[i] == 0) //  free
        {
            set_memory_block_used_when_boot(i, true);
            return i;
        }
    }

    panic("Out of memory!!!");
    return 0; // mark_reserved_blocks_as_used never reached
}
uint64_t get_a_free_block_addr()
{
    return get_a_free_block() * 4096 + HM; // mark_reserved_blocks_as_used never reached
}
uint64_t get_continuous_free_blocks(uint64_t length)
{
    uint64_t total_blocks = total_memory_blocks_count;
    uint64_t total_kernel_mem_blocks_count = (((uint64_t)&end_kernel) - HM) / 4096;

    
    bool good = true;
    uint64_t i = 0;
    uint8_t *tmp = (uint8_t *)&memory_map;
    for (i = total_kernel_mem_blocks_count; i < total_blocks; i++)
    {
        good = true;
        for (uint64_t j = 0; j < length; j++) //  free
        {
            if (tmp[i + j] != 0)
            {
                good = false;
                break;
            }
        }
        if (good)
        {
            for (uint64_t j = 0; j < length; j++)
                set_memory_block_used_when_boot(i + j, true);
            return i;
            break;
        }
        else
            continue;
    }

    panic("Out of continuous memory!!!");
    return 0; // never reached
}
uint64_t get_a_free_slot(uint64_t map_ptr, uint64_t start, uint64_t stop)
{
    uint64_t *map_int = (uint64_t *)map_ptr;
    for (uint64_t i = start; i <= stop; i++)
    {
        uint64_t tmp = (map_int[i]);
        if (tmp == 0)
            return i;
    }
    panic("No free slot!!!");
}
uint64_t get_a_free_slot_with_size(uint64_t map_addr, uint64_t start, uint64_t stop, uint64_t unit_byte_size)
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
    panic("No free slot!!!");
}
uint64_t calc_section_blocks_count(uint64_t section_start_addr, uint64_t section_stop_addr)
{
    // TODO: use align_up();
    // uint64_t section_start_addr = (uint64_t)&section_start;
    // uint64_t section_stop_addr = (uint64_t)&section_stop;
    uint64_t section_size = section_stop_addr - section_start_addr;
    uint64_t sections_count = div_ceil(section_size, 4096);
    return sections_count;
}
extern uint64_t begin_seed;
extern uint64_t end_seed;
extern const void *seed_text_start;
extern const void *seed_text_end;
extern const void *seed_data_start;
extern const void *seed_data_end;
extern const void *seed_bss_start;
extern const void *seed_bss_end;
extern const void *seed_rodata_start;
extern const void *seed_rodata_end;
extern const void *seed_heap_start;
extern const void *seed_heap_end;
uint64_t calc_process_seed_blocks_count()
{
    uint64_t process_blocks_count = 0;
    // uint64_t s = (uint64_t)&seed_text_start;
    // uint64_t e = (uint64_t)&seed_text_end;
    process_blocks_count += calc_section_blocks_count((uint64_t)&seed_text_start, (uint64_t)&seed_text_end);
    process_blocks_count += calc_section_blocks_count((uint64_t)&seed_data_start, (uint64_t)&seed_data_end);
    process_blocks_count += calc_section_blocks_count((uint64_t)&seed_bss_start, (uint64_t)&seed_bss_end);
    process_blocks_count += calc_section_blocks_count((uint64_t)&seed_rodata_start, (uint64_t)&seed_rodata_end);
    process_blocks_count += calc_section_blocks_count((uint64_t)&seed_heap_start, (uint64_t)&seed_heap_end);
    process_blocks_count += 0;
    return process_blocks_count;
}
typedef uint64_t page_line;
typedef page_line page[512];
void erase_block(uint64_t *ptr)
{
    for (uint64_t i = 0; i < 4096 / 8; i++)
        ((uint64_t *)ptr)[i] = 0;
}
void create_page_table_for_process(uint64_t pid, uint64_t blocks_count, uint64_t start_block)
{
    int64_t block_index = -1;
    //=======================================================
    page_line *current_page_l4 = (page_line *)get_a_free_block_addr();
    erase_block(current_page_l4);
    for (uint64_t i4 = 0; i4 < 128; i4++)
    {
        page_line *current_page_l3 = (page_line *)get_a_free_block_addr();
        erase_block(current_page_l3);
        current_page_l4[i4] = (uint64_t)current_page_l3 - HM | PTE_PRESENT | PTE_RW | PTE_USER;
        for (uint64_t i3 = 0; i3 < 512; i3++)
        {
            page_line *current_page_l2 = (page_line *)get_a_free_block_addr();
            erase_block(current_page_l2);
            current_page_l3[i3] = (uint64_t)current_page_l2 - HM | PTE_PRESENT | PTE_RW | PTE_USER;
            for (uint64_t i2 = 0; i2 < 512; i2++)
            {
                page_line *current_page_l1 = (page_line *)get_a_free_block_addr();
                erase_block(current_page_l1);
                current_page_l2[i2] = (uint64_t)current_page_l1 - HM | PTE_PRESENT | PTE_RW | PTE_USER;
                for (uint64_t i1 = 0; i1 < 512; i1++)
                {
                    block_index++;
                    if (block_index >= blocks_count + start_block)
                    {
                        goto end_loop;
                    }
                    if (block_index < start_block)
                        continue;
                    uint64_t new_free_block_addr = get_a_free_block_addr();
                    current_page_l1[i1] = new_free_block_addr - HM | PTE_PRESENT | PTE_RW | PTE_USER;
                }
            }
        }
    }
end_loop:
    // page_line *kernel_space_l4 = (page_line *)(((uint64_t)&page_table_l4));
    page_line *kernel_space_l4_b = (page_line *)(((uint64_t)&page_table_l4_b) + HM);
    for (uint64_t i = 0; i < 128; i++)
    {
        page_line tmp = (page_line)(kernel_space_l4_b[i]);
        current_page_l4[i + 128] = kernel_space_l4_b[i];
    }
    processes_cr3_ptr[pid] = (uint64_t)current_page_l4;
}
void setup_process_info(uint64_t pid, uint64_t parent_pid)
{
    uint64_t pid_addr = get_a_free_block_addr();
    erase_block((void *)pid_addr);
    uint64_t *tmp = (uint64_t *)&processes_info_ptr;
    tmp[pid] = pid_addr;
    process_info_t *process_info_ptr = (process_info_t *)pid_addr;
    process_info_ptr->pid = pid;
    // process_info_ptr->cr3 = cr3_addr;
    process_info_ptr->state = PROCESS_RUNNING;
    process_info_ptr->main_thread_id = 0;
    // process_info_ptr->signal = 0;
    process_info_ptr->threads_count = 0;
    uint64_t fd_addr = get_continuous_free_blocks(20) * 4096 + HM;
    process_info_ptr->fd_addr = fd_addr;
    // process_info_ptr->signal = 0;
    process_info_ptr->parent_pid = parent_pid;
    if (parent_pid == pid)
    {
        process_info_ptr->pgid = pid;
        process_info_ptr->sid = pid;
    }
    else
    {
        process_info_ptr->pgid = process_info_ptr[parent_pid].pgid;
        process_info_ptr->sid = process_info_ptr[parent_pid].sid;
    }
}
void setup_idle_process_info()
{
    uint64_t pid = 0;
    uint64_t pid_addr = get_a_free_block_addr();
    erase_block((void *)pid_addr);
    uint64_t *tmp = (uint64_t *)&processes_info_ptr;
    tmp[pid] = pid_addr;
    process_info_t *process_info_ptr = (process_info_t *)pid_addr;
    process_info_ptr->pid = pid;
    // process_info_ptr->cr3 = cr3_addr;
    process_info_ptr->state = PROCESS_IDLE;
    process_info_ptr->main_thread_id = 0;
    // process_info_ptr->signal = 0;
    process_info_ptr->threads_count = 0;
    // cr3;
    uint64_t *tmp1 = (uint64_t *)&processes_cr3_ptr;
    *tmp1 = ((uint64_t)&page_table_l4) + HM;
}
uint64_t page_line_to_addr(uint64_t pageline)
{
    
    return (pageline & 0x000FFFFFFFFFF000ULL) + HM;
}
void increase_paging_of_process(uint64_t pid)
{
    page *largest_l4_page_ptr = (page *)(processes_cr3_ptr[pid]);
    uint8_t i = 0;
    for (i = 0; i < 256; i++)
    {
        if (largest_l4_page_ptr[i] == 0)
        {
            break;
        }
    }
    uint8_t last_l4 = i - 1;
    uint64_t page_line_of_largest_l3 = (uint64_t)(largest_l4_page_ptr[last_l4]);

    page *largest_l3_page_ptr = (page *)(page_line_to_addr(page_line_of_largest_l3));
    for (i = 0; i < 512; i++)
    {
        if (largest_l3_page_ptr[i] == 0)
        {
            break;
        }
    }
    uint8_t last_l3 = i - 1;
    uint64_t page_line_of_largest_l2 = (uint64_t)(largest_l3_page_ptr[last_l3]);

    page *largest_l2_page_ptr = (page *)(page_line_to_addr(page_line_of_largest_l2));
    for (i = 0; i < 512; i++)
    {
        if (largest_l2_page_ptr[i] == 0)
        {
            break;
        }
    }
    uint8_t last_l2 = i - 1;
    uint64_t page_line_of_largest_l1 = (uint64_t)(largest_l2_page_ptr[last_l2]);

    page *largest_l1_page_ptr = (page *)(page_line_to_addr(page_line_of_largest_l1));
    for (i = 0; i < 512; i++)
    {
        if (largest_l1_page_ptr[i] == 0)
        {
            break;
        }
    }
    uint8_t last_l1 = i - 1;
    // uint64_t page_line_of_largest_l0 = l1_page_ptr[last_l4];
    // page* largest_l1_page_ptr = (page*)(page_line_to_addr(page_line_of_largest_l1));

    uint64_t temp = last_l4 * 512 * 512 * 512 + last_l3 * 512 * 512 + last_l2 * 512 + last_l1 + 1;
    uint64_t new_mem_block_addr = get_a_free_block_addr();
    uint64_t new_l1_block_addr = 0, new_l2_block_addr = 0, new_l3_block_addr = 0;
    if (temp % (512) == 0)
    {
        uint64_t new_l1_block_addr = get_a_free_block_addr();
        erase_block((void *)new_l1_block_addr);
        *((page *)new_l1_block_addr)[0] = new_mem_block_addr | PTE_PRESENT | PTE_RW | PTE_USER;
    }
    else
    {
        *largest_l1_page_ptr[last_l1 + 1] = new_mem_block_addr | PTE_PRESENT | PTE_RW | PTE_USER;
        return;
    }

    if (temp % (512 * 512) == 0)
    {
        uint64_t new_l2_block_addr = get_a_free_block_addr();
        erase_block((void *)new_l2_block_addr);
        *((page *)new_l2_block_addr)[0] = new_l1_block_addr | PTE_PRESENT | PTE_RW | PTE_USER;
    }
    else
    {
        *largest_l2_page_ptr[last_l2 + 1] = new_l1_block_addr | PTE_PRESENT | PTE_RW | PTE_USER;
        return;
    }

    if (temp % (512 * 512 * 512) == 0)
    {
        uint64_t new_l3_block_addr = get_a_free_block_addr();
        erase_block((void *)new_l3_block_addr);
        *((page *)new_l3_block_addr)[0] = new_l2_block_addr | PTE_PRESENT | PTE_RW | PTE_USER;
    }
    else
    {
        *largest_l3_page_ptr[last_l3 + 1] = new_l2_block_addr | PTE_PRESENT | PTE_RW | PTE_USER;
        return;
    }

    *largest_l4_page_ptr[last_l4 + 1] = new_l3_block_addr | PTE_PRESENT | PTE_RW | PTE_USER;
}
uint64_t increase_paging_of_process_with_virt_addr(uint64_t pid, uint64_t virt_addr)
{
    uint64_t l4_i = (virt_addr >> 39) & 0x1FF;
    uint64_t l3_i = (virt_addr >> 30) & 0x1FF;
    uint64_t l2_i = (virt_addr >> 21) & 0x1FF;
    uint64_t l1_i = (virt_addr >> 12) & 0x1FF;

    // uint64_t l4_i = 0;
    // uint64_t l3_i = 0;
    // uint64_t l2_i = 0;
    // uint64_t l1_i = 0;

    uint64_t phys_addr;
    page_line *l4_ptr = 0, *l3_ptr = 0, *l2_ptr = 0, *l1_ptr = 0;
    //=============================================
    l4_ptr = (page_line *)processes_cr3_ptr[pid];
    if (l4_ptr[l4_i] == 0)
    {
        uint64_t new_block_addr = get_a_free_block_addr();
        l3_ptr = (page_line *)new_block_addr;
        erase_block(l3_ptr);
        l4_ptr[l4_i] = (new_block_addr - HM) | PTE_PRESENT | PTE_RW | PTE_USER;
    }
    else
        l3_ptr = (page_line *)page_line_to_addr((uint64_t)(l4_ptr[l4_i]));
    //=============================================
    if (l3_ptr[l3_i] == 0)
    {
        uint64_t new_block_addr = get_a_free_block_addr();
        l2_ptr = (page_line *)new_block_addr;
        erase_block(l2_ptr);
        l3_ptr[l3_i] = (new_block_addr - HM) | PTE_PRESENT | PTE_RW | PTE_USER;
    }
    else
        l2_ptr = (page_line *)page_line_to_addr((uint64_t)(l3_ptr[l3_i]));
    //=============================================
    if (l2_ptr[l2_i] == 0)
    {
        uint64_t new_block_addr = get_a_free_block_addr();
        l1_ptr = (page_line *)new_block_addr;
        erase_block(l1_ptr);
        l2_ptr[l2_i] = (new_block_addr - HM) | PTE_PRESENT | PTE_RW | PTE_USER;
    }
    else
        l1_ptr = (page_line *)page_line_to_addr((uint64_t)(l2_ptr[l2_i]));
    //=============================================
    if (l1_ptr[l1_i] == 0)
        l1_ptr[l1_i] = (phys_addr = get_a_free_block_addr() - HM) | PTE_PRESENT | PTE_RW | PTE_USER;
    return phys_addr;
}
bool block_empty(uint64_t addr)
{
    uint64_t *tmp = (uint64_t *)addr;
    for (uint16_t i = 0; i < 512; i++)
    {
        if (tmp[i] != 0)
            return false;
    }
    return true;
}
void decrease_paging_of_process_with_virt_addr(uint64_t pid, uint64_t virt_addr)
{
    uint64_t l4_i = (virt_addr >> 39) & 0x1FF;
    uint64_t l3_i = (virt_addr >> 30) & 0x1FF;
    uint64_t l2_i = (virt_addr >> 21) & 0x1FF;
    uint64_t l1_i = (virt_addr >> 12) & 0x1FF;

    // uint64_t l4_i = 0;
    // uint64_t l3_i = 0;
    // uint64_t l2_i = 0;
    // uint64_t l1_i = 0;

    page_line *l4_ptr = 0, *l3_ptr = 0, *l2_ptr = 0, *l1_ptr = 0;
    //=============================================
    l4_ptr = (page_line *)processes_cr3_ptr[pid];
    if (l4_ptr[l4_i] == 0)
    {
        return;
    }
    else
        l3_ptr = (page_line *)page_line_to_addr((uint64_t)(l4_ptr[l4_i]));
    //=============================================
    if (l3_ptr[l3_i] == 0)
    {
        return;
    }
    else
        l2_ptr = (page_line *)page_line_to_addr((uint64_t)(l3_ptr[l3_i]));
    //=============================================
    if (l2_ptr[l2_i] == 0)
    {
        return;
    }
    else
        l1_ptr = (page_line *)page_line_to_addr((uint64_t)(l2_ptr[l2_i]));
    //=============================================
    if (l1_ptr[l1_i] == 0)
        return;

    //========================================================================
    set_memory_block_used_when_boot((uint64_t)l1_ptr[l1_i], false);
    l1_ptr[l1_i] = 0;
    if (!block_empty((uint64_t)l1_ptr))
        return;
    set_memory_block_used_when_boot((uint64_t)l2_ptr[l2_i], false);
    l2_ptr[l2_i] = 0;
    if (!block_empty((uint64_t)l2_ptr))
        return;
    set_memory_block_used_when_boot((uint64_t)l3_ptr[l3_i], false);
    l3_ptr[l3_i] = 0;
    if (!block_empty((uint64_t)l3_ptr))
        return;
    set_memory_block_used_when_boot((uint64_t)l4_ptr[l4_i], false);
    l4_ptr[l4_i] = 0;
}
bool is_paged(uint64_t pid, uint64_t virt_addr)
{
    uint64_t l4_i = (virt_addr >> 39) & 0x1FF;
    uint64_t l3_i = (virt_addr >> 30) & 0x1FF;
    uint64_t l2_i = (virt_addr >> 21) & 0x1FF;
    uint64_t l1_i = (virt_addr >> 12) & 0x1FF;
    // uint64_t l4_i = 1;
    // uint64_t l3_i = 0;
    // void *xx = &l3_i;
    // uint64_t l2_i = 0;
    // uint64_t l1_i = 1;
    page_line tmp = 0, *l4_ptr = 0, *l3_ptr = 0, *l2_ptr = 0, *l1_ptr = 0;
    l4_ptr = (uint64_t *)processes_cr3_ptr[pid];
    tmp = l4_ptr[l4_i];
    if (tmp == 0)
    {
        return false;
    }
    l3_ptr = (page_line *)(page_line_to_addr(tmp));
    tmp = l3_ptr[l3_i];
    if (tmp == 0)
    {
        return false;
    }
    l2_ptr = (page_line *)(page_line_to_addr(tmp));
    tmp = l2_ptr[l2_i];
    if (tmp == 0)
    {
        return false;
    }
    l1_ptr = (page_line *)(page_line_to_addr(tmp));
    tmp = l1_ptr[l1_i];
    if (tmp == 0)
    {
        return false;
    }
    return true;
}
uint64_t find_free_virtual_region(uint64_t pid, uint64_t length)
{
    bool flag = true;
    uint64_t blocks_count = div_ceil(length, 4096);
    for (uint64_t i = 1; i < HM / 4096 - 1; i++)
    {
        flag = true;
        for (uint64_t j = 0; j < blocks_count; j++)
        {
            if (is_paged(pid, (i + j) * 4096))
            {
                flag = false;
                break;
            }
        };
        if (flag)
            return i * 4096;
    }
    return 0;
}
extern void xsleep();
extern void *xsleep_top;
void register_idle_thread()
{
    uint64_t idle_pid = 0;
    int64_t process_info_ptr = (uint64_t)processes_info_ptr[idle_pid];
    process_info_t *this_process_ptr = (process_info_t *)process_info_ptr;
    uint64_t thread_info_addr = get_a_free_block_addr();
    thread_info_t *thread_info_ptr = (thread_info_t *)thread_info_addr;
    erase_block((void *)(thread_info_addr));
    thread_info_ptr->pid = idle_pid;
    thread_info_ptr->state = THREAD_IDLE;
    uint64_t idle_thread_id = 0;
    thread_info_ptr->tid = idle_thread_id;
    threads_info_ptr[idle_thread_id] = thread_info_ptr;
    thread_info_ptr->wakeup_time.tv_sec = 0;
    thread_info_ptr->wakeup_time.tv_microsec = 0;
    thread_info_ptr->stack_addr = 0;
    thread_info_ptr->context.rsp = (uint64_t)&xsleep_top;
    thread_info_ptr->context.rip = (uint64_t)xsleep;
    thread_info_ptr->context.rflags = 0x202;
    thread_info_ptr->context.cs = 0x8;
    thread_info_ptr->context.ss = 0x10;
    thread_info_ptr->context.rflags = 0x202;
    thread_info_ptr->kernel_stack_addr = 0;
    this_process_ptr->threads_count++;
}
uint64_t register_a_thread(uint64_t pid, uint64_t entry_point_addr)
{
    // run as kernel cr3 1 time , then run as user cr3;
    // stack of thread;
    uint64_t process_info_addr = (uint64_t)(&processes_info_ptr[pid]);
    process_info_t *this_process_ptr = (process_info_t *)process_info_addr;
    uint64_t i = 0;
    uint64_t thread_stack_top = HM - 4096;
    while (true)
    {
        if (!is_paged(pid, thread_stack_top - 4096))
            break;
        thread_stack_top -= 5 * 4096;
    }
    for (uint8_t i = 0; i < 4; i++)
    {
        increase_paging_of_process_with_virt_addr(pid, thread_stack_top - i * 4096 - 4096);
    };
    // thread_info kernel_space;
    // thread_stack_top -=16;
    uint64_t thread_info_addr = (get_continuous_free_blocks(5)) * 4096 + HM;
    thread_info_t *thread_info_ptr = (thread_info_t *)thread_info_addr;
    for (uint8_t i = 0; i < 5; i++)
        erase_block((void *)(thread_info_addr + i * 4096));
    thread_info_ptr->pid = pid;
    thread_info_ptr->state = THREAD_NOT_EXIST;
    uint64_t thread_id = get_a_free_slot((uint64_t)&threads_info_ptr, 2, MAX_THREADS_COUNT);
    thread_info_ptr->tid = thread_id;
    threads_info_ptr[thread_id] = thread_info_ptr;
    thread_info_ptr->wakeup_time.tv_sec = 0;
    thread_info_ptr->wakeup_time.tv_microsec = 0;
    thread_info_ptr->stack_addr = thread_stack_top;
    thread_info_ptr->context.rsp = thread_stack_top;
    thread_info_ptr->context.rip = (uint64_t)entry_point_addr;
    thread_info_ptr->context.rflags = 0x202;
    thread_info_ptr->context.cs = 0x23;
    thread_info_ptr->context.ss = 0x1b;
    thread_info_ptr->kernel_stack_addr = (uint64_t)(thread_info_addr) + 5 * 4096;
    thread_info_ptr->signal = 0;
    thread_info_ptr->context_type = CONTEXT_TYPE_ALU;
    this_process_ptr->threads_count++;
    return thread_id;
}
// TODO: not used
uint64_t register_a_sub_thread(uint64_t pid, uint64_t stack_top_addr, uint64_t ptid)
{
    // run as kernel cr3 1 time , then run as user cr3;
    // stack of thread;
    uint64_t entry_point_addr = *(uint64_t *)stack_top_addr;
    uint64_t process_info_ptr = (uint64_t)(&processes_info_ptr[pid]);
    process_info_t *this_process_ptr = (process_info_t *)process_info_ptr;
    uint64_t i = 0;
    uint64_t thread_info_addr = (get_continuous_free_blocks(5)) * 4096 + HM;
    thread_info_t *thread_info_ptr = (thread_info_t *)thread_info_addr;
    for (uint8_t i = 0; i < 5; i++)
        erase_block((void *)(thread_info_addr + i * 4096));
    thread_info_ptr->pid = pid;
    thread_info_ptr->state = THREAD_READY;
    uint64_t thread_id = get_a_free_slot((uint64_t)&threads_info_ptr, 1, MAX_THREADS_COUNT);
    thread_info_ptr->tid = thread_id;
    threads_info_ptr[thread_id] = thread_info_ptr;
    thread_info_ptr->wakeup_time.tv_sec = 0;
    thread_info_ptr->wakeup_time.tv_microsec = 0;
    thread_info_ptr->stack_addr = stack_top_addr;
    thread_info_ptr->context.rsp = stack_top_addr;
    thread_info_ptr->context.rip = (uint64_t)entry_point_addr;
    thread_info_ptr->context.rflags = 0x202;
    thread_info_ptr->context.cs = 0x23;
    thread_info_ptr->context.ss = 0x1b;
    thread_info_ptr->kernel_stack_addr = (uint64_t)(thread_info_addr) + 5 * 4096;
    thread_info_ptr->ptid = ptid;
    thread_info_ptr->signal = 0;
    thread_info_ptr->context_type = CONTEXT_TYPE_ALU;
    this_process_ptr->threads_count++;
    return thread_id;
}
void setup_kernel_process()
{
    uint64_t pid = 1;
    processes_cr3_ptr[pid] = ((uint64_t)&page_table_l4) + HM;
    process_info_t *proc = (process_info_t *)get_a_free_block_addr();
    processes_info_ptr[pid] = proc;
    erase_mem8((uint64_t)proc, sizeof(process_info_t));
    proc->state = PROCESS_RUNNING;
}
uint64_t register_a_kernel_thread(uint64_t entry_point_addr)
{
    uint64_t thread_info_addr = (get_continuous_free_blocks(5)) * 4096 + HM;
    uint64_t thread_stack_top = thread_info_addr + 5 * 4096;
    thread_info_t *thread_info_ptr = (thread_info_t *)thread_info_addr;
    for (uint8_t i = 0; i < 5; i++)
        erase_block((void *)(thread_info_addr + i * 4096));
    thread_info_ptr->pid = 1;
    thread_info_ptr->state = THREAD_READY;
    uint64_t thread_id = get_a_free_slot((uint64_t)&threads_info_ptr, 1, MAX_THREADS_COUNT);
    thread_info_ptr->tid = thread_id;
    threads_info_ptr[thread_id] = thread_info_ptr;
    thread_info_ptr->wakeup_time.tv_sec = 0;
    thread_info_ptr->wakeup_time.tv_microsec = 0;
    thread_info_ptr->stack_addr = thread_stack_top;
    thread_info_ptr->context.rsp = thread_stack_top;
    thread_info_ptr->context.rip = (uint64_t)entry_point_addr;
    thread_info_ptr->context.rflags = 0x202;
    thread_info_ptr->context.cs = 0x08;
    thread_info_ptr->context.ss = 0x10;
    thread_info_ptr->kernel_stack_addr = (uint64_t)(thread_info_addr) + 5 * 4096;
    thread_info_ptr->signal = 0;
    thread_info_ptr->context_type = CONTEXT_TYPE_ALU;
    return thread_id;
}
void kernel_thread_sleep(uint64_t duration_us)
{
    
    uint64_t tid = get_current_thread_id();
    uint64_t x = (uint64_t)threads_info_ptr[tid];
    thread_info_t *thread = threads_info_ptr[tid];

    
    spin_lock(&threads_info_lock2);
    struct timeval now = get_time_us();
    struct timeval wt = add_time_us(now, (uint32_t)duration_us);
    memcpy((uint8_t *)&thread->wakeup_time, (uint8_t *)&wt, sizeof(struct timeval));

    
    thread->state = THREAD_BLOCKED_OTHER;

    
    spin_unlock(&threads_info_lock2);
    scheduler_yeild();
    // return;
}
void kernel_thread_sleep_on_network(uint64_t addr)
{
    
    uint64_t tid = get_current_thread_id();
    uint64_t x = (uint64_t)threads_info_ptr[tid];
    thread_info_t *thread = threads_info_ptr[tid];

    
    thread->state = THREAD_BLOCKED_NETWORK;
    thread->sleep_flag_addr = addr;

    
    tick_interrupt_handler1();
}
void copy_seed_to_memory(uint64_t addr)
{
    void *dst = (void *)addr;
    memcpy((uint8_t *)dst, (uint8_t *)&begin_seed, (uint8_t *)&end_seed - (uint8_t *)&begin_seed);
    for (uint64_t i = (uint64_t)&seed_bss_start; i < (uint64_t)&seed_bss_end; i++)
        *(uint8_t *)i = 0;
}

extern void change_cr3(int);
void switch_to_process_cr3(uint64_t app_cr3_addr)
{
    // void*tmp = (void*) app_cr3_addr;
    // asm volatile("mov %0, %%cr3" ::"r"(app_cr3_addr - HM) : "memory");
    change_cr3(app_cr3_addr - HM);
    // uint64_t a= 1;
    // uint64_t b= 2;
}
void switch_to_kernel_cr3()
{
    // void*tmp = (void*) app_cr3_addr;
    // asm volatile("mov %0, %%cr3" ::"r"(app_cr3_addr - HM) : "memory");
    change_cr3((uint64_t)&page_table_l4);
    // uint64_t a= 1;
    // uint64_t b= 2;
}
extern void *testt;
void test(void *input)
{
    uint64_t *tmp = (uint64_t *)&memory_map;
    uint64_t a = get_a_free_block();
    uint64_t b = get_a_free_block();
    uint64_t c = get_a_free_block();
    uint64_t d = get_a_free_block();
    uint64_t e = get_a_free_block();
}
void start_process_seed_from_ram()
{
    uint64_t pid = get_a_free_slot((uint64_t)&processes_info_ptr, 2, MAX_PROCESSES_COUNT);
    uint64_t seed_blocks_count = calc_process_seed_blocks_count();
    create_page_table_for_process(pid, seed_blocks_count, 1);
    uint64_t tmp = (uint64_t)&processes_cr3_ptr[pid];
    tmp = *(uint64_t *)tmp;
    switch_to_process_cr3(tmp);
    setup_process_info(pid, pid);
    copy_seed_to_memory(0x1000);
    // Elf64_Ehdr *ehdr = (Elf64_Ehdr *)0x0;
    // uint64_t entry_point = ehdr->e_entry;
    uint64_t entry_point = 0x1000;
    uint64_t tid = register_a_thread(pid, entry_point);
    threads_info_ptr[tid]->state = THREAD_READY;
}
void start_idle_process()
{
    setup_idle_process_info();
    register_idle_thread();
}
//========================================
struct timespec;
struct timeval;
void user_thread_current_sleep(struct timespec *req)
{
    uint64_t tid = get_current_thread_id();
    thread_info_t *t = threads_info_ptr[tid];
    t->state = THREAD_BLOCKED_OTHER;
    struct timeval reqv;
    reqv.tv_sec = req->tv_sec;
    reqv.tv_microsec = req->tv_nanosec / 1000ULL;
    struct timeval curr_time = get_time_us();
    struct timeval wake_time = add_time(curr_time, reqv);
    t->wakeup_time.tv_sec = wake_time.tv_sec;
    t->wakeup_time.tv_microsec = wake_time.tv_microsec;
    scheduler_yeild();
}
void sleep_by_resource(uint64_t flag_addr, thread_state_t state)
{
    uint64_t tid = get_current_thread_id();
    thread_info_t **tmp = threads_info_ptr;
    tmp[tid]->sleep_flag_addr = flag_addr;
    if (state == 0)
        state = THREAD_BLOCKED_OTHER;
    tmp[tid]->state = state;
    scheduler_yeild();
    return;
}
void mark_process_as_ended(uint64_t return_code)
{
    uint64_t pid = get_current_pid();
    spin_lock(&threads_info_lock1);
    process_info_t **tmp = processes_info_ptr;
    tmp[pid]->state = PROCESS_ENDED_BUT_NOT_CLEANED;
    thread_info_t **tmp1 = threads_info_ptr;
    for (uint64_t i = 0; i < MAX_THREADS_COUNT; i++)
    {
        if (!tmp1[i])
            continue;
        if (tmp1[i]->pid == pid && tmp1[i]->state != THREAD_NOT_EXIST && tmp1[i]->state != THREAD_ENDED_BUT_NOT_CLEANED)
            tmp1[i]->state = THREAD_ENDED_BUT_NOT_CLEANED;
    }
    spin_unlock((uint64_t *)&threads_info_lock1);
}
void mark_current_thread_as_ended(uint64_t return_code)
{
    uint64_t tid = get_current_thread_id();
    uint64_t pid = get_current_pid();
    spin_lock(&threads_info_lock1);
    thread_info_t **tmp1 = threads_info_ptr;
    tmp1[tid]->state = THREAD_ENDED_BUT_NOT_CLEANED;
    tmp1[tid]->return_code = return_code;
    spin_unlock(&threads_info_lock1);
    // clear cr3;
    for (uint64_t i = 1; i <= 4; i++)
        decrease_paging_of_process_with_virt_addr(pid, tmp1[tid]->stack_addr - i * 4096);
    process_info_t *proc = processes_info_ptr[get_current_pid()];
    proc->threads_count--;
    if (proc->threads_count <= 0)
        mark_process_as_ended(pid);
}
void end_thread(uint64_t tid, uint64_t return_code)
{
    thread_info_t *thread_info_ptr = threads_info_ptr[tid];
    thread_info_ptr->state = THREAD_DEAD;
    if (return_code != -2)
        thread_info_ptr->return_code = return_code;
    for (uint8_t i = 0; i < 4; i++)
        set_memory_block_used_when_boot(((uint64_t)&thread_info_ptr) / 4096 + i + 1, false);
}
void free_block(uint64_t block_addr)
{
    uint64_t block_id = div_ceil(block_addr, 4096);
    set_memory_block_used_when_boot(block_id, false);
}
void free_page(uint64_t page_addr, uint8_t lv)
{
    uint64_t *page_ptr = (uint64_t *)page_addr;
    uint64_t page_entry_addr = 0;
    for (uint16_t i = 0; i < 512; i++)
    {
        page_entry_addr = page_line_to_addr(page_ptr[i]) - HM;
        if (lv > 1)
            free_page(page_entry_addr, lv - 1);
        else
            free_block(page_entry_addr);
    }
    free_block(page_addr);
}
uint64_t pageline_to_low_addr(uint64_t page_line);
void free_page_hm(uint64_t page_addr, uint8_t lv)
{
    uint64_t *page_ptr = (uint64_t *)page_addr;
    uint64_t page_entry_addr = 0;
    uint16_t max_pageline = lv == 4 ? HM >> 39 : 512;
    for (uint16_t i = 0; i < max_pageline; i++)
    {
        page_entry_addr = pageline_to_low_addr(page_ptr[i]);
        if (page_entry_addr == 0)
            continue;
        page_entry_addr += HM;
        if (lv > 1)
            free_page_hm(page_entry_addr, lv - 1);
        else
            free_block(page_entry_addr);
    }
    free_block(page_addr);
}
void stop_process(uint64_t pid)
{
    for (uint64_t i = 0; i < MAX_THREADS_COUNT; i++)
    {
        thread_info_t *curr = threads_info_ptr[i];
        if ((uint64_t)curr != 0)
            continue;
        if (curr->pid > pid)
            curr->state = THREAD_WAITING_SIGNAL;
    };
}
void end_process(uint64_t pid, uint8_t return_code)
{
    uint64_t process_cr3_addr = processes_cr3_ptr[pid];
    uint64_t *c3_ptr = (uint64_t *)(process_cr3_addr);
    free_page_hm((uint64_t)c3_ptr, 4);
    process_cr3_addr = 0;
    process_info_t *process_info_ptr = (process_info_t *)(processes_info_ptr[pid]);
    process_info_ptr->state = PROCESS_STOPPED;
    set_memory_block_used_when_boot(process_info_ptr->fd_addr / 4096, false);
    set_memory_block_used_when_boot(process_info_ptr->fd_addr / 4096 + 1, false);
    // assign all children to PID 1
    for (uint64_t i = 0; i < MAX_PROCESSES_COUNT; i++)
    {
        if (processes_info_ptr[i] == 0)
            continue;
        if (processes_info_ptr[i]->parent_pid == pid)
            processes_info_ptr[i]->parent_pid = 1;
    }
    process_info_ptr->fd_addr = 0;
    process_info_ptr->return_code = return_code;
    // count children
    uint64_t count_children_running = 0;
    for (uint64_t i = 0; i < MAX_PROCESSES_COUNT; i++)
    {
        process_info_t *tmp_proc = processes_info_ptr[i];
        if (tmp_proc != 0 && tmp_proc && tmp_proc->parent_pid == pid &&
            tmp_proc->state != PROCESS_STOPPED &&
            tmp_proc->state != PROCESS_ENDED_BUT_NOT_CLEANED &&
            tmp_proc->state != PROCESS_NOT_EXIST)
            count_children_running++;
    }
    // handle parent waiting;
    uint64_t parent_pid = process_info_ptr->parent_pid;
    for (uint64_t i = 0; i < MAX_THREADS_COUNT; i++)
    {
        thread_info_t *tmp_thread = threads_info_ptr[i];
        if (!tmp_thread || tmp_thread->pid != parent_pid || tmp_thread->state != THREAD_BLOCKED_PID)
            continue;
        if (tmp_thread->sleep_flag_addr == -1 && count_children_running == 0)
        {
            tmp_thread->state = THREAD_READY;
            tmp_thread->sleep_flag_addr = 0;
        }
        if (tmp_thread->sleep_flag_addr == (uint64_t)&process_info_ptr->lock)
        {
            tmp_thread->state = THREAD_READY;
            tmp_thread->sleep_flag_addr = 0;
        }
    };
}
#endif // CORE_H
