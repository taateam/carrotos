#ifndef ELF_LOADER_H
#define ELF_LOADER_H

#include <stdint.h>
#include <stdbool.h>
#include "../filesytem/fat32.h"
#define MAX_PHT_PER_ELF 16
#define AT_NULL 0    /* end of vector */
#define AT_IGNORE 1  /* entry should be ignored */
#define AT_EXECFD 2  /* file descriptor of program */
#define AT_PHDR 3    /* program headers for program */
#define AT_PHENT 4   /* size of program header entry */
#define AT_PHNUM 5   /* number of program headers */
#define AT_PAGESZ 6  /* system page size */
#define AT_BASE 7    /* base address of interpreter */
#define AT_FLAGS 8   /* flags */
#define AT_ENTRY 9   /* entry point of program */
#define AT_UID 11    /* real uid */
#define AT_EUID 12   /* effective uid */
#define AT_GID 13    /* real gid */
#define AT_EGID 14   /* effective gid */
#define AT_EXECFN 31 
#define AT_RANDOM 25 

// ELF Program Header Types
#define PT_NULL 0    // unused entry
#define PT_LOAD 1    // loadable segment
#define PT_DYNAMIC 2 // dynamic linking info
#define PT_INTERP 3  // interpreter info (/ld.so)
#define PT_NOTE 4    // auxiliary info
#define PT_SHLIB 5   // reserved
#define PT_PHDR 6    // program header table itself
#define PT_TLS 7     // thread-local storage segment

struct elf_header_t
{
    unsigned char e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed));
struct pht_entry_t
{
    uint32_t p_type;   /* Segment type */
    uint32_t p_flags;  /* Segment flags */
    uint64_t p_offset; /* Segment file offset */
    uint64_t p_vaddr;  /* Segment virtual address */
    uint64_t p_paddr;  /* Segment physical address */
    uint64_t p_filesz; /* Segment size in file */
    uint64_t p_memsz;  /* Segment size in memory */
    uint64_t p_align;  /* Segment alignment */
} __attribute__((packed));
struct auxv
{
    uint64_t a_type;
    uint64_t a_val;
};
bool is_elf(const uint8_t *buffer)
{
    return buffer[0] == 0x7F &&
           memcmp(buffer + 1, "ELF", 3) == 0;
};
void extend_cr3(uint64_t pid, uint64_t virtual_mem_offset, uint64_t pht_size_in_mem)
{
    for (uint64_t i = virtual_mem_offset / 4096 * 4096; i <= virtual_mem_offset + pht_size_in_mem; i += 4096)
    {
        increase_paging_of_process_with_virt_addr(pid, i);
    }
};
uint64_t get_cr3()
{
    uint64_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}
uint64_t load_pht_to_memory(char *path, struct pht_entry_t *pht_e, uint64_t pid, uint64_t start_offset)
{
    if (pht_e->p_type != 1)
        return 0;
    uint64_t file_offset = pht_e->p_offset;
    uint64_t virtual_mem_offset = pht_e->p_vaddr + start_offset;
    uint64_t pht_size_in_file = pht_e->p_filesz;
    uint64_t pht_size_in_mem = pht_e->p_memsz;
    extend_cr3(pid, virtual_mem_offset, pht_size_in_mem);
    if (pht_size_in_file > pht_size_in_mem)
        return -1;
    int rs = read_file(path, (uint8_t *)virtual_mem_offset, file_offset, pht_size_in_file);
    if (rs < 0)
        return -2;
    if (pht_size_in_mem > pht_size_in_file)
    {
        erase_mem8(virtual_mem_offset + pht_size_in_file, pht_size_in_mem - pht_size_in_file);
    }
    return pht_size_in_mem;
};
uint64_t load_so(char *path, uint64_t pid, uint64_t start_offset)
{
    uint8_t buffer_elf[4];
    read_file(path, buffer_elf, 0, 4);
    if (!is_elf(buffer_elf))
        return -1;
    struct DirentStat elf_file_stat = stat_dirent_ret(path);
    uint64_t elf_file_size = elf_file_stat.size;
    uint64_t elf_header_size = sizeof(struct elf_header_t);
    uint8_t buffer_elf_header[elf_header_size];
    read_file(path, buffer_elf_header, 0, elf_header_size);
    struct elf_header_t *elf_header = (struct elf_header_t *)buffer_elf_header;
    uint64_t pht_entries_count = elf_header->e_phnum;
    if (pht_entries_count > MAX_PHT_PER_ELF)
        return -2; // error;
    uint64_t pht_entry_size = sizeof(struct pht_entry_t);
    uint8_t buffer_pht_entries[pht_entries_count * pht_entry_size];
    read_file(path, buffer_pht_entries, elf_header->e_phoff, pht_entries_count * pht_entry_size);
    struct pht_entry_t *pht_entry = (struct pht_entry_t *)buffer_pht_entries;
    // uint64_t pid = create_process();
    // switch_to_cr3(pid);
    for (uint64_t i = 0; i < pht_entries_count; i++)
    {
        load_pht_to_memory(path, &pht_entry[i], pid, start_offset);
    };
    return elf_header->e_entry + start_offset;
};
uint64_t count_strings(char *arr[])
{
    uint64_t count = 0;
    if (!arr)
        return 0; 
    while (arr[count] != 0)
    {
        count++;
    }
    return count;
}
uint64_t align_up(uint64_t num, uint64_t alignment)
{
    return div_ceil(num, alignment) * alignment;
}
uint64_t align_down_16_8(uint64_t num)
{
    uint64_t align_16 = num / 16 * 16;
    if (num % 16 < 8)
        align_16 -= 16;
    return align_16 + 8;
}
uint64_t init_stack(uint64_t main_thread_id, char *argv[], char *envp[], uint64_t phdr_addr, uint64_t count_loaded_sections, uint64_t ld_so_entry, struct elf_header_t *elf_header)
{
    thread_info_t *main_thread = threads_info_ptr[main_thread_id];
    uint64_t current_rsp = main_thread->stack_addr - 8;
    // ======================== agruments
    uint64_t argc = count_strings(argv);
    char *argv_ptr[argc];
    for (uint64_t i = 0; i < argc; i++)
    {
        current_rsp -= strlen(argv[i]) + 1;
        argv_ptr[i] = (char *)current_rsp;
        memcpy((uint8_t *)current_rsp, argv[i], strlen(argv[i]) + 1);
    }
    char *arg0_ptr = (char *)0;
    if (argc > 0)
    {
        arg0_ptr = argv_ptr[0];
    }
    current_rsp -= 8;
    *(uint64_t *)current_rsp = 0; // null terminator;
    // =================== environment variables
    uint64_t envc = count_strings(envp);
    char *envp_ptr[envc];
    for (uint64_t i = 0; i < envc; i++)
    {
        current_rsp -= strlen(envp[i]) + 1;
        envp_ptr[i] = (char *)current_rsp;
        memcpy((uint8_t *)current_rsp, envp[i], strlen(envp[i]) + 1);
    }
    current_rsp -= 8;
    *(uint64_t *)current_rsp = 0; // null terminator;
    // =========== place 16 bytes random for AT_RANDOM (must be actual bytes)
    uint64_t random_bytes_addr = current_rsp;
    for (int i = 0; i < 16; ++i)
    {
        ((uint8_t *)current_rsp)[i] = (uint8_t)(rand() & 0xFF); // or a nicer RNG
    }
    uint64_t random_16_bytes_addr = current_rsp;
    // ==============================padding
    struct auxv auxv_default[] = {
        {AT_PHDR, phdr_addr},
        {AT_PHENT, sizeof(struct pht_entry_t)},
        {AT_PHNUM, count_loaded_sections},
        {AT_ENTRY, elf_header->e_entry},
        {AT_PAGESZ, 0x1000},
        
        {AT_BASE, ld_so_entry},
        // ★ stack random (16 bytes)
        {AT_RANDOM, random_16_bytes_addr},
        
        {AT_EXECFN, (uint64_t)arg0_ptr},
        
        {AT_UID, 0},
        {AT_EUID, 0},
        {AT_GID, 0},
        {AT_EGID, 0},
        
        {AT_NULL, 0}};
    uint64_t rsp_bottom = current_rsp;
    rsp_bottom -= sizeof(auxv_default);
    rsp_bottom -= sizeof(argv_ptr);
    rsp_bottom -= sizeof(envp_ptr);
    rsp_bottom -= 8;  // argc;
    rsp_bottom -= 16; // 2 null after argv_ptr and envp_ptr;
    uint64_t rsp_bottom_aligned = align_down_16_8(rsp_bottom);
    uint64_t padding_size = rsp_bottom - rsp_bottom_aligned;
    current_rsp -= padding_size;
    erase_mem8(current_rsp, padding_size);
    // ================== auxiliary
    current_rsp -= sizeof(auxv_default);
    memcpy((uint8_t *)current_rsp, (uint8_t *)&auxv_default, sizeof(auxv_default));
    // ================agrument pointers
    current_rsp -= 8;
    *(uint64_t *)current_rsp = 0; // null;
    current_rsp -= sizeof(argv_ptr);
    memcpy((uint8_t *)current_rsp, (uint8_t *)&argv_ptr, sizeof(argv_ptr));
    // ================environment variable pointers
    current_rsp -= 8;
    *(uint64_t *)current_rsp = 0; // null;
    current_rsp -= sizeof(envp_ptr);
    memcpy((uint8_t *)current_rsp, (uint8_t *)&envp_ptr, sizeof(envp_ptr));
    // =========================== rsp
    current_rsp -= 8;
    *(uint64_t *)current_rsp = argc;
    current_rsp -= 8;
    *(uint64_t *)current_rsp = 0;
    main_thread->context.rsp = current_rsp;
}
bool allow_exec_path(char *path);
bool allow_read_path(char *path);
uint64_t run_elf_process(char *path, char *argv[], char *envp[], bool is_fork)
{
    if (!allow_exec_path(path) || !allow_read_path(path))
        return -EACCES;
    uint8_t buffer_elf[4];
    read_file(path, buffer_elf, 0, 4);
    if (!is_elf(buffer_elf))
        return -1;
    struct DirentStat elf_file_stat = stat_dirent_ret(path);
    uint64_t elf_file_size = elf_file_stat.size;
    uint64_t elf_header_size = sizeof(struct elf_header_t);
    uint8_t buffer_elf_header[elf_header_size];
    read_file(path, buffer_elf_header, 0, elf_header_size);
    struct elf_header_t *elf_header = (struct elf_header_t *)buffer_elf_header;
    uint64_t pht_entries_count = elf_header->e_phnum;
    if (pht_entries_count > MAX_PHT_PER_ELF)
        return -2; // error;
    uint64_t pht_entry_size = sizeof(struct pht_entry_t);
    uint8_t buffer_pht_entries[pht_entries_count * pht_entry_size];
    read_file(path, buffer_pht_entries, elf_header->e_phoff, pht_entries_count * pht_entry_size);
    struct pht_entry_t *pht_entry = (struct pht_entry_t *)buffer_pht_entries;
    // processes_info + threads_info
    uint64_t pid;
    if (!is_fork)
    {
        pid = get_a_free_slot((uint64_t)&processes_info_ptr, 0, MAX_PROCESSES_COUNT);
        setup_process_info(pid, pid);
    }
    else
    {
        pid = get_current_pid();
    }
    create_page_table_for_process(pid, 1, 1);
    uint64_t cr3_tmp = processes_cr3_ptr[pid];
    // tmp = *(uint64_t *)tmp;
    switch_to_process_cr3(cr3_tmp);
    uint64_t last_memory_offset = 0;
    uint64_t count_loaded_sections = 0;
    for (uint64_t i = 0; i < pht_entries_count; i++)
    {
        uint64_t count_bytes_copied = load_pht_to_memory(path, &pht_entry[i], pid, 0);
        if (count_bytes_copied == -1 || count_bytes_copied == -2)
            return count_bytes_copied;
        if (count_bytes_copied == 0)
            continue;
        count_loaded_sections++;
        last_memory_offset += count_bytes_copied;
    };
    // ========= check if ld.so is needed;
    const char *interp_path = NULL;
    for (uint64_t i = 0; i < pht_entries_count; i++)
    {
        struct pht_entry_t *ph = &pht_entry[i];
        if (ph->p_type == PT_INTERP)
        {
            // PT_INTERP segment offset = p_offset, size = p_filesz
            uint8_t buffer_interp[ph->p_filesz + 1];
            read_file(path, buffer_interp, ph->p_offset, ph->p_filesz);
            buffer_interp[ph->p_filesz] = 0; // null-terminate
            interp_path = (const char *)buffer_interp;
            break;
        }
    }
    //=================================
    uint64_t thread_entry = elf_header->e_entry;
    if (interp_path)
    {
        thread_entry = load_so("/ld.so", pid, div_ceil(last_memory_offset, 4096) * 4096);
        if (thread_entry == -1 || thread_entry == -2)
            return thread_entry;
    }
    change_cr3((uint64_t)&page_table_l4);
    uint64_t main_thread_id = register_a_thread(pid, thread_entry);
    ((process_info_t *)&processes_info_ptr)[pid].main_thread_id = main_thread_id;
    ((process_info_t *)&processes_info_ptr)[pid].state = PROCESS_IDLE;
    // prepare stack;
    uint64_t phdr_addr = elf_header->e_phoff;
    //==============================================================
    //====================== STACK =================================
    //==============================================================
    uint64_t cr3_backup = get_cr3();
    switch_to_process_cr3(cr3_tmp);
    init_stack(main_thread_id, argv, envp, phdr_addr, count_loaded_sections, thread_entry, elf_header);
    change_cr3(cr3_backup);
    //=====================================================================
    //================ end STACK ==========================================
    //=====================================================================
    processes_info_ptr[pid]->signal = 0;
    uint64_t heap_start = 0;
    for (uint64_t i = 0; i < pht_entries_count; i++)
    {
        struct pht_entry_t *ph = &pht_entry[i];
        if (ph->p_type != PT_LOAD)
            continue;

        uint64_t seg_end = ph->p_vaddr + ph->p_memsz;
        if (seg_end > heap_start)
            heap_start = seg_end;
    }

    
    heap_start = align_up(heap_start, 0x1000);
    uint64_t heap_end = heap_start; 
    processes_info_ptr[pid]->heap_start_addr = heap_start;
    processes_info_ptr[pid]->heap_end_addr = heap_end;
    threads_info_ptr[main_thread_id]->state = THREAD_READY;
    return pid;
};
#endif // { ELF_LOADER_H }
