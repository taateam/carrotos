#ifndef EXE_LOADER_H
#define EXE_LOADER_H

#include "common.h"
uint64_t setup_peb_and_teb0(uint64_t new_pid, char *main_app_path, uint64_t new_main_thread, uint64_t *current_top_mem_addr, uint64_t main_base_addr, uint64_t ntdll_base_addr, uint64_t ninja_base_addr, char *argv, char *envp, char *cwd)
{
    // use ansi path not unicode
    const uint8_t MAX_BLOCKS_FOR_PEB_TEB = 4;
    for (char i = 0; i < MAX_BLOCKS_FOR_PEB_TEB; i++)
        increase_paging_of_process_with_virt_addr(new_pid, *current_top_mem_addr + i * 4096);
    uint64_t new_alloc_addr = *current_top_mem_addr;
    *current_top_mem_addr += MAX_BLOCKS_FOR_PEB_TEB * 4096;
    struct PEB *new_peb = (struct PEB *)new_alloc_addr;
    new_peb->SessionId = 0;
    // setup list of dlls loaded - PEB_LDR_DATA
    struct list_loaded_dlls *loaded_dlls_list = (struct list_loaded_dlls *)(new_alloc_addr + sizeof(struct PEB));
    loaded_dlls_list->Length = sizeof(struct list_loaded_dlls);
    loaded_dlls_list->Initialized = 1;
    loaded_dlls_list->SsHandle = NULL;
    // some headers
    new_peb->ImageBaseAddress = (void *)main_base_addr;
    // heap
    // for (char i = 0; i < 4; i++)
    // {
    //     increase_paging_of_process_with_virt_addr(new_pid, HEAP_START + i * 4096);
    // }
    // new_peb->ProcessHeap = (void *)HEAP_START;

    // argv and envc
    ustr512 argv_u, envp_u;
    uint64_t argv_len = strlen(argv);
    uint64_t envp_len = strlen(envp);
    uint64_t cwd_len = strlen(cwd);
    uint64_t path_len = strlen(main_app_path);
    if (argv_len + envp_len + cwd_len + path_len + 4 > MAX_ARGV_ENVP_CWD_LEN)
    {
        // Not enough space in PEB buffer
        return -1;
    }
    uint16_t *argv_in_buffer = (uint16_t *)(uint64_t)&new_peb->argv_envp_cwd_buf;
    new_peb->process_params_buf.CommandLine.MaximumLength = 512;
    new_peb->process_params_buf.CommandLine.Buffer = (uint16_t *)argv_in_buffer;
    ustrcpy_from_str(&new_peb->process_params_buf.CommandLine, argv);
    //--------------------------
    uint16_t *envp_in_buffer = (uint16_t *)((uint64_t)&new_peb->argv_envp_cwd_buf + 1024);
    utf8_to_utf16(envp, envp_len, (uint16_t *)envp_in_buffer, MAX_ARGV_ENVP_CWD_LEN);
    envp_in_buffer[envp_len] = '\0';
    new_peb->process_params_buf.Environment = (void *)envp_in_buffer;
    //--------------------------
    uint16_t *cwd_in_buffer = (uint16_t *)((uint64_t)&new_peb->argv_envp_cwd_buf + 2 * 1024);
    new_peb->process_params_buf.CurrentDirectoryPath.MaximumLength = 512;
    new_peb->process_params_buf.CurrentDirectoryPath.Buffer = (uint16_t *)cwd_in_buffer;
    ustrcpy_from_str(&new_peb->process_params_buf.CurrentDirectoryPath, cwd);
    new_peb->process_params->DllPath.Buffer = new_peb->process_params->CommandLine.Buffer;
    //--------------------------
    uint16_t *path = (uint16_t *)((uint64_t)&new_peb->argv_envp_cwd_buf + 3 * 1024);
    new_peb->process_params_buf.ImagePathName.MaximumLength = 512;
    new_peb->process_params_buf.ImagePathName.Buffer = (uint16_t *)path;
    ustrcpy_from_str(&new_peb->process_params_buf.ImagePathName, main_app_path);
    //--------------------------
    new_peb->process_params = &new_peb->process_params_buf;
    //=====================================================
    uint64_t max_loaded_dll_data = MAX_DLL_PER_PROCESS;
    struct loaded_dll *loaded_dll = (struct loaded_dll *)*current_top_mem_addr;
    for (uint64_t i = 0; i < MAX_DLL_LIST_BLOCKS_COUNT; i++)
    {
        increase_paging_of_process_with_virt_addr(new_pid, *current_top_mem_addr);
        erase_block((void *)(*current_top_mem_addr));
        *current_top_mem_addr += 4096;
    }
    // process main app info in dll list;
    loaded_dlls_list->InLoadOrderModuleList.next = 0;
    loaded_dlls_list->InLoadOrderModuleList.prev = 0;
    loaded_dlls_list->InMemoryOrderModuleList.next = 0;
    loaded_dlls_list->InMemoryOrderModuleList.prev = 0;
    loaded_dlls_list->InInitializationOrderModuleList.next = 0;
    loaded_dlls_list->InInitializationOrderModuleList.prev = 0;

    add_loaded_dll(0, main_app_path, loaded_dlls_list, loaded_dll, main_base_addr);
    add_loaded_dll(1, "ntdll.dll", loaded_dlls_list, loaded_dll, ntdll_base_addr);
    add_loaded_dll(2, "ninja.dll", loaded_dlls_list, loaded_dll, ninja_base_addr);
    new_peb->loaded_dlls = loaded_dlls_list;
    //======================================================
    // setup TEB
    struct TEB *new_teb0 = (struct TEB *)(new_alloc_addr + sizeof(struct PEB) + sizeof(struct list_loaded_dlls));
    new_teb0->NtTib.ExceptionList = (void *)0x12345678;
    new_teb0->ClientIdUniqueProcess = (void *)new_pid;
    new_teb0->ClientIdUniqueThread = (void *)new_main_thread;
    new_teb0->NtTib.StackBase = (void *)(HM - 4096);
    new_teb0->NtTib.StackLimit = (void *)(HM - 4096 - 5 * 4096);
    new_teb0->NtTib.Self = (void *)new_teb0;
    //--------
    new_peb->FastPebLock = &new_peb->peb_lock;
    memset(&new_peb->peb_lock, 0, sizeof(new_peb->peb_lock));
    new_peb->peb_lock.LockCount = -1;
    // tls bitmap
    new_peb->TlsBitmap = &new_peb->TlsBitmapBuffer;
    new_peb->TlsBitmapBuffer[0] = 0;
    new_peb->TlsBitmapBuffer[1] = 0;
    // link PEB to TEB
    new_teb0->ProcessEnvironmentBlock = new_peb;
    return (uint64_t)new_teb0;
}
//=============================================================================
int64_t pe_load_section_to_memory(char *path, struct pe_section_header *section_header, uint64_t image_base)
{
    uint64_t size_of_raw_data = section_header->size_of_raw_data;
    uint64_t pointer_to_raw_data = section_header->pointer_to_raw_data;
    uint64_t virtual_size = section_header->virtual_size;
    uint64_t virtual_address = section_header->virtual_address + image_base;
    uint64_t copy_size = size_of_raw_data;
    if (copy_size > virtual_size)
        copy_size = virtual_size;
    // load section data from file to memory
    if (size_of_raw_data > 0)
    {
        int rs = read_file(path, (uint8_t *)(virtual_address), pointer_to_raw_data, copy_size);
        if (rs < 0)
            return rs;
    }
    // if virtual_size > size_of_raw_data, erase the remaining bytes in memory
    if (virtual_size > size_of_raw_data)
    {
        erase_mem8(virtual_address + size_of_raw_data, virtual_size - size_of_raw_data);
    }
    return 0;
};
bool pe_image_relocation(struct pe_optional_header64 *opt_header64, uint64_t dll_base_memory_addr, int64_t offset_distance)
{
    struct data_directory reloc_directory = opt_header64->data_directories[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    if (reloc_directory.size == 0)
        return true; // no relocation info
    uint64_t reloc_table_base_addr = reloc_directory.virtual_address + dll_base_memory_addr;
    uint64_t reloc_section_byte_offset = 0;
    while (reloc_section_byte_offset < reloc_directory.size)
    {
        struct pe_relocation_block *reloc_block = (struct pe_relocation_block *)(reloc_table_base_addr + reloc_section_byte_offset);
        uint64_t entry_count = (reloc_block->block_size - sizeof(struct pe_relocation_block)) / 2;
        if (reloc_block->block_size < sizeof(*reloc_block))
            break;
        reloc_section_byte_offset += reloc_block->block_size;
        for (uint64_t i = 0; i < entry_count; i++)
        {
            uint16_t *entries = (uint16_t *)((uint8_t *)reloc_block + sizeof(*reloc_block));
            uint16_t entry = entries[i];
            uint16_t entry_type = entry >> 12;
            uint16_t entry_offset = entry & 0x0FFF;
            if (entry_type == 10)
            {
                // IMAGE_REL_BASED_DIR64
                uint64_t *patch_addr = (uint64_t *)(reloc_block->page_rva + entry_offset + dll_base_memory_addr);
                *patch_addr += offset_distance;
            }
            else if (entry_type == 3)
            {
                // IMAGE_REL_BASED_HIGHLOW
                uint32_t *patch_addr = (uint32_t *)(reloc_block->page_rva + entry_offset + dll_base_memory_addr);
                *patch_addr += (uint32_t)offset_distance;
            }
            // else ignore other types
        }
    }
    return true;
}
void setup_kuser_shared_data(uint64_t pid)
{
    increase_paging_of_process_with_virt_addr(pid, KUSER_SHARED_DATA);
    increase_paging_of_process_with_virt_addr(pid, KUSER_SHARED_DATA + 4096);
    erase_block((void *)KUSER_SHARED_DATA);
    erase_block((void *)KUSER_SHARED_DATA + 4096);
    struct kuser_shared_data *kuser_data = (struct kuser_shared_data *)KUSER_SHARED_DATA;
    kuser_data->SystemCallFlag = 0;       
    kuser_data->SystemCallDispatcher = 0; 
}
static uint64_t ntdll_base_addr = 0;
static uint64_t stack_ntdll = 0x3fffffffefd0ULL;
static uint64_t entry_ntdll = 0;

uint32_t *pe_find_export_addr_pointer_by_name(char *import_file_name, struct data_directory *sub_export_directory, uint64_t dll_base_memory_addr)
{
    // find the export table of the dll;
    struct pe_export_directory *export_table = (struct pe_export_directory *)(sub_export_directory->virtual_address + dll_base_memory_addr);
    uint64_t max_export = export_table->number_of_names;
    uint64_t i = 0;
    for (; i < max_export; i++)
    {
        uint32_t *name_rvas = (uint32_t *)(dll_base_memory_addr + export_table->address_of_names);
        char *export_name = (char *)(dll_base_memory_addr + name_rvas[i]);
        if (strcmp(export_name, import_file_name))
        {
            {
                break;
            }
        }
    }
    if (i == max_export)
        return 0;
    uint16_t *name_ordinals = (uint16_t *)(dll_base_memory_addr + export_table->address_of_name_ordinals);
    uint32_t *function_addresses = (uint32_t *)(dll_base_memory_addr + export_table->address_of_functions);
    uint16_t ordinal_index = name_ordinals[i];
    uint32_t *function_rva_address = &function_addresses[ordinal_index];
    return function_rva_address;
}
struct pe_optional_header64 *get_opt_header64(uint64_t loaded_base_addr)
{
    struct dos_header_simplified *dos_header = (struct dos_header_simplified *)loaded_base_addr;
    uint32_t pe_header_offset = dos_header->e_lfanew;
    struct pe_file_header *pe_file_header = (struct pe_file_header *)(pe_header_offset + 4 + loaded_base_addr);
    return (struct pe_optional_header64 *)(pe_header_offset + 4 + sizeof(struct pe_file_header) + loaded_base_addr);
};
uint64_t get_base_address_from_peb(uint64_t dll_id)
{
    uint64_t peb_addr = 0;
    asm volatile("mov %%gs:0x60, %0" : "=r"(peb_addr));
    struct PEB *peb = (struct PEB *)peb_addr;
    struct list_loaded_dlls *loaded_dlls_list = (struct list_loaded_dlls *)peb->loaded_dlls;

    // struct loaded_dll *current = (struct loaded_dll *)loaded_dlls_list->InLoadOrderModuleList.next;
    // uint64_t index = 0;
    // while (current != (struct loaded_dll *)&loaded_dlls_list->InLoadOrderModuleList)
    // {
    //     if (index == dll_id)
    //     {
    //         return current->DllBase;
    //     }
    //     current = (struct loaded_dll *)current->InLoadOrderLinks.next;
    //     index++;
    // }

    // fast
    struct loaded_dll *current = (struct loaded_dll *)((uint64_t)peb + 4096 * 4);
    if (&current[dll_id].InLoadOrderLinks.next != 0)
        return (uint64_t)current[dll_id].DllBase;
    return 0;
}
void patch_export(uint64_t ntdll_base_addr, uint64_t ninja_base_addr)
{
    struct pe_optional_header64 *ntdll_opt_header64 = get_opt_header64(ntdll_base_addr);
    struct pe_optional_header64 *ninja_opt_header64 = get_opt_header64(ninja_base_addr);
    struct pe_export_directory *ntdll_export_directory = (struct pe_export_directory *)(ntdll_opt_header64->data_directories[IMAGE_DIRECTORY_ENTRY_EXPORT].virtual_address + ntdll_base_addr);
    struct pe_export_directory *ninja_export_directory = (struct pe_export_directory *)(ninja_opt_header64->data_directories[IMAGE_DIRECTORY_ENTRY_EXPORT].virtual_address + ninja_base_addr);
    uint8_t *tmp = (uint8_t *)(&ntdll_opt_header64->data_directories[IMAGE_DIRECTORY_ENTRY_EXPORT]);
    struct data_directory *ntdll_all_exports_dir = (struct data_directory *)tmp;
    uint64_t i = 0;
    for (; i < ninja_export_directory->number_of_names; i++)
    {
        uint32_t ninja_exp_name_rva = ((uint32_t *)(ninja_export_directory->address_of_names + ninja_base_addr))[i];
        char *ninja_exp_name = (char *)(ninja_exp_name_rva + ninja_base_addr);
        if (!ninja_exp_name)
            break;
        // Patch the export address
        uint16_t *ninja_name_ordinals = (uint16_t *)(ninja_base_addr + ninja_export_directory->address_of_name_ordinals);
        uint32_t *ninja_function_addresses = (uint32_t *)(ninja_base_addr + ninja_export_directory->address_of_functions);
        uint16_t ninja_ordinal_index = ninja_name_ordinals[i];
        uint32_t ninja_exp_rva = ninja_function_addresses[ninja_ordinal_index];
        uint32_t *ntdll_need_to_be_patched_addr = pe_find_export_addr_pointer_by_name(ninja_exp_name, ntdll_all_exports_dir, ntdll_base_addr);
        if (ntdll_need_to_be_patched_addr != 0)
        {
            // Patch the export address
            *ntdll_need_to_be_patched_addr = ninja_exp_rva + ninja_base_addr - ntdll_base_addr;
        }
    }
}
int64_t load_dll_by_name(char *dll_file_name, uint64_t main_image_base, uint64_t *dll_base_memory_addr, uint64_t new_pid)
{
    ntdll_base_addr = *dll_base_memory_addr;
    uint64_t ide_channel = 0, part_id = 0; // Load the DLL into memory
    uint64_t dll_file_name_len = strlen(dll_file_name);
    if (dll_file_name_len == 0)
        return -1;
    char full_dll_path[256];
    char *dll_folder = "/dll/";
    strconcat(full_dll_path, dll_folder, dll_file_name);
    uint8_t buf_dos_header[64];
    read_file(full_dll_path, buf_dos_header, 0, 64);
    struct dos_header_simplified *dos_header = (struct dos_header_simplified *)buf_dos_header;
    if (dos_header->e_magic != 0x5A4D)
        return -ENOEXEC; // not a valid exe file
    uint32_t pe_header_offset = dos_header->e_lfanew;
    uint8_t buf_pe_signature_and_file_header[24];
    read_file(full_dll_path, buf_pe_signature_and_file_header, pe_header_offset, 24);
    if (memcmp(buf_pe_signature_and_file_header, "PE\0\0", 4) != 0)
        return -ENOEXEC; // not a valid PE file
    struct pe_file_header *pe_file_header = (struct pe_file_header *)(buf_pe_signature_and_file_header + 4);
    uint16_t opt_size = pe_file_header->size_of_optional_header;
    uint8_t buf_optional[opt_size];
    memset(buf_optional, 0, sizeof(buf_optional));
    read_file(full_dll_path, buf_optional, pe_header_offset + 4 + sizeof(struct pe_file_header), opt_size);
    uint16_t magic = *(uint16_t *)buf_optional;

    if (magic != 0x20B)
    {
        return -ENOEXEC; // not a valid PE file
    }
    struct pe_optional_header64 *opt_header64 = (struct pe_optional_header64 *)buf_optional;
    // page
    extend_cr3(new_pid, *dll_base_memory_addr, opt_header64->size_of_image);
    // load headers
    read_file(full_dll_path, (uint8_t *)*dll_base_memory_addr, 0, opt_header64->size_of_headers);
    // Load sections;
    for (uint16_t i = 0; i < pe_file_header->number_of_sections; i++)
    {
        struct pe_section_header section_header;
        // if (strcmp(section_header.name, "/4"))
        //     echo("x");
        read_file(full_dll_path, (uint8_t *)&section_header, pe_header_offset + 4 + sizeof(struct pe_file_header) + opt_size + i * sizeof(struct pe_section_header), sizeof(struct pe_section_header));
        uint64_t res = pe_load_section_to_memory(full_dll_path, &section_header, *dll_base_memory_addr);
        if (res < 0)
        {
            echo(section_header.name);
            panic(": Load section to memory failed");
        }
    }
    // get LdrInitializeThunk;
    struct data_directory export_directory = opt_header64->data_directories[IMAGE_DIRECTORY_ENTRY_EXPORT];
    struct image_export_directory *exp =
        (struct image_export_directory *)(*dll_base_memory_addr + export_directory.virtual_address);
    uint32_t *func_rvas = (uint32_t *)(*dll_base_memory_addr + exp->AddressOfFunctions);

    uint32_t *name_rvas = (uint32_t *)(*dll_base_memory_addr + exp->AddressOfNames);

    uint16_t *ordinals = (uint16_t *)(*dll_base_memory_addr + exp->AddressOfNameOrdinals);
    uint64_t rtl_user_thread_start = 0;

    // uint64_t milestone = 5369195332ULL;
    // uint64_t smallest = 0;
    // char last_name[64];
    // for (uint32_t i = 0; i < exp->NumberOfNames; i++)
    // {
    //     uint16_t ord = ordinals[i];
    //     uint32_t rva = func_rvas[ord];

    //     rtl_user_thread_start =
    //         *dll_base_memory_addr + rva;
    //     if (rtl_user_thread_start < milestone && rtl_user_thread_start > smallest)
    //     {
    //         smallest = rtl_user_thread_start;
    //         char *name = (char *)(*dll_base_memory_addr + name_rvas[i]);
    //         strcpy(last_name, name);
    //     }
    // }
    // echo (last_name);
    for (uint32_t i = 0; i < exp->NumberOfNames; i++)
    {
        char *name = (char *)(*dll_base_memory_addr + name_rvas[i]);

        if (strcmp(name, "LdrInitializeThunk"))
        {
            uint16_t ord = ordinals[i];
            uint32_t rva = func_rvas[ord];

            rtl_user_thread_start =
                *dll_base_memory_addr + rva;
            break;
        }
    };
    // relocation;
    int64_t offset_distance = opt_header64->image_base - *dll_base_memory_addr;
    if (!pe_image_relocation(opt_header64, *dll_base_memory_addr, offset_distance))
        return -1;
    entry_ntdll = rtl_user_thread_start;
    *dll_base_memory_addr += div_ceil(opt_header64->size_of_image, 4096) * 4096;
    return rtl_user_thread_start;
}
uint64_t run_exe_process(char *path, char *argv, char *envp, char *current_dir)
{
    if (!allow_exec_path(path) || !allow_read_path(path))
        return -EACCES;
    // check if is exe
    uint8_t buf_dos_header[64];
    read_file(path, buf_dos_header, 0, 64);
    struct dos_header_simplified *dos_header = (struct dos_header_simplified *)buf_dos_header;
    if (dos_header->e_magic != 0x5A4D)
        return -ENOEXEC; // not a valid exe file
    uint32_t pe_header_offset = dos_header->e_lfanew;
    uint8_t buf_pe_signature_and_file_header[24];
    read_file(path, buf_pe_signature_and_file_header, pe_header_offset, 24);
    if (memcmp(buf_pe_signature_and_file_header, "PE\0\0", 4) != 0)
        return -ENOEXEC; // not a valid PE file
    struct pe_file_header *pe_file_header = (struct pe_file_header *)(buf_pe_signature_and_file_header + 4);
    uint16_t opt_size = pe_file_header->size_of_optional_header;
    uint8_t buf_optional[opt_size];
    memset(buf_optional, 0, sizeof(buf_optional));
    read_file(path, buf_optional, pe_header_offset + 4 + sizeof(struct pe_file_header), opt_size);
    uint16_t magic = *(uint16_t *)buf_optional;

    if (magic != 0x20B)
    {
        return -ENOEXEC; // not a valid PE file
    }
    struct pe_optional_header64 *opt_header64 = (struct pe_optional_header64 *)buf_optional;
    // process info and allocate memory
    uint64_t new_pid = get_a_free_slot((uint64_t)&processes_info_ptr, 0, MAX_PROCESSES_COUNT);
    process_info_t *new_proc = processes_info_ptr[new_pid];
    setup_process_info(new_pid, new_pid);
    uint64_t process_start_block = opt_header64->image_base / 4096;
    uint64_t process_end_block = div_ceil(opt_header64->image_base + opt_header64->size_of_image, 4096);
    uint64_t process_blocks_count = process_end_block - process_start_block;
    create_page_table_for_process(new_pid, process_blocks_count, process_start_block);
    uint64_t cr3_tmp = processes_cr3_ptr[new_pid];
    uint64_t new_main_thread = register_a_thread(new_pid, 0);
    new_proc->main_thread_id = new_main_thread;
    // argv;
    char argv_hm[strlen(argv) + 1];
    strcpy(argv_hm, argv);
    char envp_hm[strlen(envp) + 1];
    strcpy(envp_hm, envp);
    char current_dir_hm[strlen(current_dir) + 1];
    strcpy(current_dir_hm, current_dir);
    // cr3
    uint64_t cr3_backup = get_cr3();
    switch_to_process_cr3(cr3_tmp);
    // load headers
    read_file(path, (uint8_t *)opt_header64->image_base, 0, opt_header64->size_of_headers);
    //=====load sections========
    for (uint16_t i = 0; i < pe_file_header->number_of_sections; i++)
    {
        struct pe_section_header section_header;
        read_file(path, (uint8_t *)&section_header, pe_header_offset + 4 + sizeof(struct pe_file_header) + opt_size + i * sizeof(struct pe_section_header), sizeof(struct pe_section_header));
        uint64_t res = pe_load_section_to_memory(path, &section_header, opt_header64->image_base);
        if (res < 0)
            panic("Load section to memory failed");
    }
    uint64_t current_top_mem_addr = div_ceil(opt_header64->image_base + opt_header64->size_of_image, 4096) * 4096;
    // load ntdll;
    uint64_t ntdll_base_addr = current_top_mem_addr;
    char *ntdll_file_name = "ntdll.dll";
    int64_t ntdll_entry_point = load_dll_by_name(ntdll_file_name, opt_header64->image_base, &current_top_mem_addr, new_pid);
    if (ntdll_entry_point < 0)
        panic("Load ntdll.dll failed");
    // load ninja;
    uint64_t ninja_base_addr = current_top_mem_addr;
    char *ninja_file_name = "ninja.dll";
    int64_t ninja_entry_point = load_dll_by_name(ninja_file_name, opt_header64->image_base, &current_top_mem_addr, new_pid);
    if (ninja_entry_point < 0)
        panic("Load ninja.dll failed");
    patch_export(ntdll_base_addr, ninja_base_addr);
    // setup peb
    uint64_t peb_addr = current_top_mem_addr;
    uint64_t teb_addr = setup_peb_and_teb0(new_pid, path, new_main_thread, &current_top_mem_addr, opt_header64->image_base, ntdll_base_addr, ninja_base_addr, argv_hm, envp_hm, current_dir_hm);
    // setup teb;
    threads_info_ptr[new_main_thread]->context.gs_base = teb_addr;
    // stack
    uint64_t *stack_top = (uint64_t *)(HM - 4096);
    stack_top -= 1;
    *stack_top = 0x12345678; // fake return addr;
    stack_top -= 1;
    *stack_top = peb_addr; // PEB addr in RDX;
    stack_top -= 1;
    struct PEB *peb = (struct PEB *)peb_addr;
    memcpy((uint8_t *)*stack_top, (uint8_t *)peb->process_params->CommandLine.Buffer, wstrlen(peb->process_params->CommandLine.Buffer) * 2 + 2); // argv in RCX;
    stack_top -= wstrlen(peb->process_params->CommandLine.Buffer) * 2 + 2;
    memcpy((uint8_t *)*stack_top, (uint8_t *)peb->process_params->Environment, wstrlen(peb->process_params->Environment) * 2 + 2); // envp in R8;
    stack_top -= wstrlen(peb->process_params->Environment) * 2 + 2;
    // =================================
    setup_kuser_shared_data(new_pid);
    // =================================
    switch_to_process_cr3(cr3_backup);
    threads_info_ptr[new_main_thread]->context.rip = ninja_entry_point;
    threads_info_ptr[new_main_thread]->context_type = CONTEXT_TYPE_SSE;
    threads_info_ptr[new_main_thread]->state = THREAD_READY;
    return 0;
}
#endif // { EXE_LOADER_H }
