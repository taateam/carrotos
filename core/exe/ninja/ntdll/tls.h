#ifndef EXE_TLS_H
#define EXE_TLS_H

#define MAX_THREAD_PER_PROCESS 256

struct image_tls_directory64
{
    uint64_t StartAddressOfRawData; // VA
    uint64_t EndAddressOfRawData;   // VA
    uint64_t AddressOfIndex;        // uint32_t* (TLS index)
    uint64_t AddressOfCallBacks;    // VA -> PIMAGE_TLS_CALLBACK[]
    uint32_t SizeOfZeroFill;
    uint32_t Characteristics;
};
uint64_t pe_load_section_to_memory1(char *path, struct pe_section_header *section_header, uint64_t image_base)
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
uint64_t alloc_pages(uint64_t num_pages, uint64_t current_top_mem_addr)
{
    extend_cr3(current_top_mem_addr, num_pages * 4096);
    return current_top_mem_addr;
};
int64_t find_dll_loaded(char *dll_file_name)
{
    uint16_t dll_file_name_utf16[strlen(dll_file_name) + 1];
    utf8_to_utf16(dll_file_name, strlen(dll_file_name) + 1, dll_file_name_utf16, strlen(dll_file_name) + 1);
    uint64_t peb_addr = 0;
    asm volatile("mov %%gs:0x60, %0" : "=r"(peb_addr));
    struct PEB *peb = (struct PEB *)peb_addr;
    struct list_loaded_dlls *loaded_dlls_list = (struct list_loaded_dlls *)peb->loaded_dlls;

    // struct loaded_dll *current = (struct loaded_dll *)loaded_dlls_list->InLoadOrderModuleList.next;
    // uint64_t index = 0;
    // while (current != (struct loaded_dll *)&loaded_dlls_list->InLoadOrderModuleList)
    // {
    //     char debug1buf[128];
    //     memset(debug1buf, 0, 128);
    //     utf16_to_utf8(current->name, ustrlen(&current->BaseDllName), debug1buf, 128);
    //     char *debug = debug1buf;
    //     if (strcmp_iw(current->name, dll_file_name_utf16))
    //     {
    //         return index;
    //     }
    //     current = (struct loaded_dll *)current->InLoadOrderLinks.next;
    //     index++;
    // }

    struct loaded_dll *dlls = (uint64_t)peb + 4 * 4096;
    for (uint64_t i = 0; i < MAX_THREAD_PER_PROCESS; i++)
    {
        struct loaded_dll *curr_dll = &dlls[i];
        if (!curr_dll->DllBase)
            continue;
        if (strcmp_iw(curr_dll->name, dll_file_name_utf16))
            return i;
    }
    return -1;
};
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
        return current[dll_id].DllBase;
    return 0;
}
int64_t process_tls(struct image_tls_directory64 *tls, uint64_t tls_id, uint64_t *current_top_mem_addr, uint32_t *tls_id_buf);
uint64_t pe_iat_process(char *dll_file_name, struct data_directory *main_import_directory, struct pe_optional_header64 *opt_header64, uint64_t main_image_base, uint64_t dll_base_memory_addr);
bool pe_image_relocation1(struct pe_optional_header64 *opt_header64, uint64_t main_image_base, uint64_t dll_base_memory_addr, int64_t offset_distance);
int64_t add_pdata_entry_when_load_image(struct data_directory *exception_directory, uint64_t image_base_after_relocation, uint64_t image_size);
uint64_t strlen(const char *s);

struct pe_section_header *get_rdata_section(uint64_t loaded_dll_id)
{
    uint64_t loaded_base_addr = get_base_address_from_peb(loaded_dll_id);
    struct dos_header_simplified *dos_header = (struct dos_header_simplified *)loaded_base_addr;
    uint32_t pe_header_offset = dos_header->e_lfanew;
    struct pe_file_header *pe_file_header = (struct pe_file_header *)(pe_header_offset + 4 + loaded_base_addr);
    struct pe_optional_header64 *opt_header64 = (struct pe_optional_header64 *)(pe_header_offset + 4 + sizeof(struct pe_file_header) + loaded_base_addr);
    struct pe_section_header *section_headers = (struct pe_section_header *)((uint64_t)opt_header64 + pe_file_header->size_of_optional_header);
    for (uint16_t i = 0; i < pe_file_header->number_of_sections; i++)
    {
        if (strcmp_i(section_headers[i].name, ".rdata"))
        {
            return &section_headers[i];
        }
    }
    return NULL;
}
struct pe_section_header *get_text_section(uint64_t loaded_dll_id)
{
    uint64_t loaded_base_addr = get_base_address_from_peb(loaded_dll_id);
    struct dos_header_simplified *dos_header = (struct dos_header_simplified *)loaded_base_addr;
    uint32_t pe_header_offset = dos_header->e_lfanew;
    struct pe_file_header *pe_file_header = (struct pe_file_header *)(pe_header_offset + 4 + loaded_base_addr);
    struct pe_optional_header64 *opt_header64 = (struct pe_optional_header64 *)(pe_header_offset + 4 + sizeof(struct pe_file_header) + loaded_base_addr);
    struct pe_section_header *section_headers = (struct pe_section_header *)((uint64_t)opt_header64 + pe_file_header->size_of_optional_header);
    for (uint16_t i = 0; i < pe_file_header->number_of_sections; i++)
    {
        if (strcmp_i(section_headers[i].name, ".text"))
        {
            return &section_headers[i];
        }
    }
    return NULL;
}
uint32_t *pe_find_export_addr_pointer_by_name1(char *import_file_name, struct data_directory *sub_export_directory, uint64_t dll_base_memory_addr)
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
void patch_export1(uint64_t ntdll_base_addr, uint64_t ninja_base_addr)
{
    struct pe_optional_header64 *ntdll_opt_header64 = get_opt_header64(ntdll_base_addr);
    struct pe_optional_header64 *ninja_opt_header64 = get_opt_header64(ninja_base_addr);
    struct pe_export_directory *ntdll_export_directory = (struct pe_export_directory *)(ntdll_opt_header64->data_directories[IMAGE_DIRECTORY_ENTRY_EXPORT].virtual_address + ntdll_base_addr);
    struct pe_export_directory *ninja_export_directory = (struct pe_export_directory *)(ninja_opt_header64->data_directories[IMAGE_DIRECTORY_ENTRY_EXPORT].virtual_address + ninja_base_addr);
    struct data_directory *ntdll_all_exports_dir = (struct data_directory *)(&ntdll_opt_header64->data_directories[IMAGE_DIRECTORY_ENTRY_EXPORT]);
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
        uint32_t *ntdll_need_to_be_patched_addr = pe_find_export_addr_pointer_by_name1(ninja_exp_name, ntdll_all_exports_dir, ntdll_base_addr);
        if (ntdll_need_to_be_patched_addr != 0)
        {
            // Patch the export address
            *ntdll_need_to_be_patched_addr = ninja_exp_rva + ninja_base_addr - ntdll_base_addr;
        }
    }
}
int64_t load_dll(char *dll_file_name, uint64_t main_image_base, uint64_t *dll_base_memory_addr,
                 struct data_directory *iat_directory, uint64_t *tls_id, uint32_t *tls_id_buf)
{
    // char *dll_file_name = (char *)(import_descriptor->name + main_image_base);
    //====================================
    echo(dll_file_name);
    echo(":\n");
    //====================================
    int64_t loaded_id = find_dll_loaded(dll_file_name);
    if (loaded_id != -1)
    {
        uint64_t loaded_base_addr = get_base_address_from_peb(loaded_id);
        pe_iat_process(dll_file_name, iat_directory, get_opt_header64(loaded_base_addr), main_image_base, loaded_base_addr);
        return 0;
    }
    uint64_t dll_file_name_len = strlen(dll_file_name);
    if (dll_file_name_len == 0)
        return -1;
    char full_dll_path[256];
    char *dll_path_prefix = "";
    strconcat(full_dll_path, dll_path_prefix, dll_file_name);
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
    int64_t offset_distance = *dll_base_memory_addr - opt_header64->image_base;
    extend_cr3(*dll_base_memory_addr, opt_header64->size_of_image);
    // load header
    read_file(full_dll_path, (uint8_t *)*dll_base_memory_addr, 0, opt_header64->size_of_headers);
    // Load sections;
    // struct pe_section_header text_section_header = {0};
    for (uint16_t i = 0; i < pe_file_header->number_of_sections; i++)
    {
        struct pe_section_header section_header;
        uint64_t xxxx = 5394555049ULL - *dll_base_memory_addr;
        read_file(full_dll_path, (uint8_t *)&section_header, pe_header_offset + 4 + sizeof(struct pe_file_header) + opt_size + i * sizeof(struct pe_section_header), sizeof(struct pe_section_header));
        if (memcmp(dll_file_name, "msvcrt.dll", 11) == 0 && xxxx > section_header.virtual_address && xxxx < section_header.virtual_address + section_header.virtual_size)
        {
            echo(section_header.name);
        }
        int64_t res = pe_load_section_to_memory1(full_dll_path, &section_header, *dll_base_memory_addr);
        if (res < 0)
        {
            echo(dll_file_name);
            echo(": Load section to memory failed");
        }
        // if (strcmp(section_header.name, ".text"))
        // {
        //     memcpy(&text_section_header, &section_header, sizeof(struct pe_section_header));
        //     uint64_t text_start = text_section_header.virtual_address + *dll_base_memory_addr;
        //     uint64_t text_stop = text_start + text_section_header.virtual_size;
        // }
    }
    // relocation;
    if (!pe_image_relocation1(opt_header64, main_image_base, *dll_base_memory_addr, offset_distance))
        return -1;
    // iat;
    patch_export1(*dll_base_memory_addr, get_base_address_from_peb(2));
    pe_iat_process(dll_file_name, iat_directory, opt_header64, main_image_base, *dll_base_memory_addr);
    // map new dll to peb imported dll list;
    uint64_t peb_addr = 0;
    asm volatile("mov %%gs:0x60, %0" : "=r"(peb_addr));
    struct PEB *peb = (struct PEB *)peb_addr;
    struct loaded_dll *loaded_dll = (struct loaded_dll *)((uint8_t *)peb + 4 * 4096);
    *tls_id = *tls_id + 1;
    add_loaded_dll(*tls_id, dll_file_name, peb->loaded_dlls, loaded_dll, *dll_base_memory_addr);
    // load sub dlls if any;
    struct data_directory iat_directory1 = opt_header64->data_directories[IMAGE_DIRECTORY_ENTRY_IMPORT];
    struct pe_image_import_descriptor *sub_import_descriptor = opt_header64->data_directories[IMAGE_DIRECTORY_ENTRY_IMPORT].virtual_address + *dll_base_memory_addr;
    uint64_t original_dll_base_addr = *dll_base_memory_addr;
    *dll_base_memory_addr += div_ceil(opt_header64->size_of_image, 4096) * 4096;
    while (1)
    {
        if (!sub_import_descriptor || sub_import_descriptor->name == 0)
            break;
        // uint64_t dll_starting_addr = div_ceil(dll_base_memory_addr + opt_header64->size_of_image, 4096) * 4096;
        char *debug = sub_import_descriptor->name + original_dll_base_addr;
        int64_t lib_loaded = load_dll(sub_import_descriptor->name + original_dll_base_addr, original_dll_base_addr, dll_base_memory_addr, &iat_directory1, tls_id, tls_id_buf);
        if (lib_loaded < 0)
        {
            echo("Load dll failed: ");
            echo(debug);
        }
        sub_import_descriptor++;
    }
    struct exception_directory *dll_exc_dir = (struct exception_directory *)(opt_header64->data_directories[IMAGE_DIRECTORY_ENTRY_EXCEPTION].virtual_address + original_dll_base_addr);
    add_pdata_entry_when_load_image(dll_exc_dir, *dll_base_memory_addr, opt_header64->size_of_image);

    return 0;
}
int64_t process_tls(struct image_tls_directory64 *tls, uint64_t tls_id, uint64_t *current_top_mem_addr, uint32_t *tls_id_buf)
{
    if (tls->StartAddressOfRawData == 0 || tls->EndAddressOfRawData == 0)
    {
        return 0;
    }
    tls->AddressOfIndex = &tls_id_buf[tls_id];
    tls_id_buf[tls_id] = tls_id;
    uint64_t size_of_tls_data = tls->EndAddressOfRawData - tls->StartAddressOfRawData;
    // Allocate memory for TLS data;
    *current_top_mem_addr = div_ceil(*current_top_mem_addr, 4096) * 4096;
    uint64_t tls_block_count = div_ceil(size_of_tls_data, 4096);
    uint64_t tls_memory_addr = alloc_pages(tls_block_count, *current_top_mem_addr);
    *current_top_mem_addr += tls_block_count * 4096;
    // Copy TLS data to allocated memory;
    memcpy((void *)tls_memory_addr, (void *)tls->StartAddressOfRawData, size_of_tls_data);
    // *tls_id = *tls_id + 1;
    uint64_t teb_addr = 0;
    asm volatile("mov %%gs:0x30, %0" : "=r"(teb_addr));
    struct TEB *teb = (struct TEB *)teb_addr;
    if (tls_id < 64)
    {
        teb->TlsSlots[tls_id] = tls_memory_addr;
    }
    else
    {
        tls_id_buf[tls_id - 64] = tls_memory_addr;
    }
    // Process TLS callbacks
    if (tls->AddressOfCallBacks != 0)
    {
        // Call each callback function
        for (uint64_t *callback = (uint64_t *)tls->AddressOfCallBacks; *callback != 0; callback++)
        {
            ((void (*)(uint64_t, uint64_t, uint64_t))(*callback))(get_base_address_from_peb(tls_id), 1, 0);
        }
    }
    return 1;
};
#endif // { EXE_TLS_H }
