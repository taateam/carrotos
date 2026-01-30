#include <stdint.h>
#include <stdbool.h>
#include "errorno.h"
#include "libs.h"
struct forwarded_import
{
    uint64_t import_addr;
    char *import_str;
};
struct forwarded_import forwarded_imports[32768ULL];
#include "ntdll_libs.h"
#include "tls.h"
#include "seh.h"
#include "crt.h"
typedef uint64_t size_t;
__declspec(dllexport);
typedef void (*entry1_fn_t)(void);
// void crt_init_c()
// {
//     uint64_t peb_addr = 0;
//     asm volatile("mov %%gs:0x60, %0" : "=r"(peb_addr));
//     struct PEB *peb = (struct PEB *)peb_addr;
//     uint64_t image_id = 0; // find_dll_loaded("ntdll.dll");
//     uint64_t image_base = get_base_address_from_peb(image_id);
//     struct pe_optional_header64 *opt_header = get_opt_header64(image_base);
//     // struct data_directory *export_dir_header = &opt_header->data_directories[IMAGE_DIRECTORY_ENTRY_EXPORT];
//     // struct pe_export_directory *export_dir = export_dir_header->virtual_address + image_base;
//     // uint64_t real_addr = pe_find_export_real_address_by_name1("main", export_dir_header, image_base);
//     uint64_t real_addr = opt_header->address_of_entry + image_base;
//     entry1_fn_t entry_point = (entry1_fn_t)real_addr;
//     // entry_point();
// }
typedef void (*entry2_fn_t)(uint64_t, uint64_t, uint64_t);
void run_dll_entry_point(uint64_t base_addr)
{
    struct pe_optional_header64 *opt_header = get_opt_header64(base_addr);
    uint64_t entry_rva = opt_header->address_of_entry;
    if (!entry_rva)
        return;
    uint64_t entry_addr = opt_header->address_of_entry + base_addr;
    entry2_fn_t entry_point = (entry2_fn_t)entry_addr;
    entry_point(base_addr, 1, 0);
}
void run_dll_entry_point1(uint64_t base_addr)
{
    struct pe_optional_header64 *opt_header = get_opt_header64(base_addr);
    uint64_t entry_rva = opt_header->address_of_entry;
    if (!entry_rva)
        return;
    uint64_t entry_addr = opt_header->address_of_entry + base_addr;
    entry2_fn_t entry_point = (entry2_fn_t)entry_addr;
    entry_point(base_addr, 1, 0);
}
void find_and_process_tls(uint64_t dll_id, uint64_t *dll_base_memory_addr, uint32_t *tls_id_buf)
{
    // tls
    uint64_t base_addr = get_base_address_from_peb(dll_id);
    struct pe_optional_header64 *opt_header64 = get_opt_header64(base_addr);
    if (!opt_header64->data_directories[IMAGE_DIRECTORY_ENTRY_TLS].virtual_address)
        return;
    struct image_tls_directory64 *tls_directory = (struct image_tls_directory64 *)(opt_header64->data_directories[IMAGE_DIRECTORY_ENTRY_TLS].virtual_address + base_addr);
    process_tls(tls_directory, dll_id, dll_base_memory_addr, tls_id_buf);
}
void resolve_forwarded_import(struct forwarded_import *import, int64_t loaded_dll_id, const char *func_name)
{
    uint64_t loaded_dll_base = get_base_address_from_peb(loaded_dll_id);
    struct pe_optional_header64 *loaded_dll_opt_header64 = get_opt_header64(loaded_dll_base);
    struct data_directory *loaded_dll_export_directory = &loaded_dll_opt_header64->data_directories[IMAGE_DIRECTORY_ENTRY_EXPORT];
    // struct pe_section_header *text_section = get_text_section(loaded_dll_id);
    int64_t corresponding_export = pe_find_export_rva_by_name1((char *)func_name, loaded_dll_export_directory, loaded_dll_base);
    if (corresponding_export < 0)
    {
        echo("Cannot find function in forwarded import dll\n");
        return;
    }
    if (corresponding_export < loaded_dll_export_directory->virtual_address || corresponding_export >= loaded_dll_export_directory->virtual_address + loaded_dll_export_directory->size)
    {
        *(uint64_t *)(import->import_addr) = corresponding_export + loaded_dll_base;
        import->import_addr = 0;
        import->import_str = NULL;
    }
    else
    {
        // import->import_addr = 0;
        import->import_str = (char *)corresponding_export + loaded_dll_base;
    };
    // mark as resolved
}
void run_crt(uint64_t dll_id)
{
    void *all_crt_function_ptr[1024];
    uint64_t base_addr = get_base_address_from_peb(dll_id);
    struct pe_optional_header64 *opt_header = get_opt_header64(base_addr);
    uint64_t base_end = base_addr + opt_header->size_of_image;
    struct pe_section_header *text_section = get_text_section(dll_id);
    uint64_t text_start = text_section->virtual_address + base_addr;
    uint64_t text_stop = text_start + text_section->virtual_size;
    struct pe_section_header *rdata_section = get_rdata_section(dll_id);
    uint64_t rdata_start = rdata_section->virtual_address + base_addr;
    uint64_t count_continuous_valid_function_ptr = 0;
    void *continuous_crt_function_ptr[1024];
    for (uint64_t i = 0; i < rdata_section->virtual_size / 8; i++)
    {
        if (i == 0x2de0 / 8 && dll_id == 5)
        {
            int y = 3 + 3;
        }
        uint64_t *possible_crt_function_ptr = (uint64_t *)(rdata_start + i * 8);
        if (*possible_crt_function_ptr >= text_start && *possible_crt_function_ptr < text_stop)
        {
            count_continuous_valid_function_ptr++;
            entry2_fn_t crt_function = (entry2_fn_t)(*possible_crt_function_ptr);
            uint64_t id = get_a_free_slot(continuous_crt_function_ptr, 0, 1024);
            continuous_crt_function_ptr[id] = crt_function;
        }
        else if (*possible_crt_function_ptr == 0)
        {
            continue;
        }
        else
        {
            if (count_continuous_valid_function_ptr > 3)
            {
                for (uint64_t j = 0; j < 1024; j++)
                {
                    if (!continuous_crt_function_ptr[j])
                        continue;
                    uint64_t id1 = get_a_free_slot(all_crt_function_ptr, 0, 1024);
                    all_crt_function_ptr[id1] = continuous_crt_function_ptr[j];
                }
            }
            count_continuous_valid_function_ptr = 0;
            memset(&continuous_crt_function_ptr, 0, sizeof(continuous_crt_function_ptr));
            continue;
        }
    }
    for (uint64_t i = 0; i < 1024; i++)
    {
        if (!all_crt_function_ptr[i])
            continue;
        ((void (*)())all_crt_function_ptr[i])();
    }
}
void LdrInitializeThunk()
{
    asm volatile("mov %%gs:0x60, %0" : "=r"(peb_addrx));
    tttx = (struct loaded_dll *)(peb_addrx + 4 * 4096);
    //----------------------------
    uint64_t peb_addr = 0;
    asm volatile("mov %%gs:0x60, %0" : "=r"(peb_addr));
    struct PEB *peb = (struct PEB *)peb_addr;
    uint64_t teb_addr = 0;
    asm volatile("mov %%gs:0x30, %0" : "=r"(teb_addr));
    struct TEB *teb = (struct TEB *)teb_addr;
    uint64_t tls_id = 2;

    uint64_t main_image_base = get_base_address_from_peb(0);
    struct pe_optional_header64 *main_opt_header64 = get_opt_header64(main_image_base);
    uint64_t top_addr = main_image_base + main_opt_header64->size_of_image + 0x1000000;
    uint64_t *tls_id_buf = (uint64_t *)top_addr;
    extend_cr3(top_addr, 4096 * 2);
    erase_mem8(top_addr, 4096 * 2);
    top_addr += 4096 * 2;
    teb->ThreadLocalStoragePointer = tls_id_buf;
    // if (main_opt_header64->data_directories[IMAGE_DIRECTORY_ENTRY_TLS].virtual_address)
    // {
    // struct image_tls_directory64 *main_tls_dir = (struct data_directory *)(main_opt_header64->data_directories[IMAGE_DIRECTORY_ENTRY_TLS].virtual_address + main_image_base);
    // process_tls(main_tls_dir, &tls_id, pid, &top_addr, tls_id_buf);
    struct exception_directory *main_exc_dir = (struct exception_directory *)(main_opt_header64->data_directories[IMAGE_DIRECTORY_ENTRY_EXCEPTION].virtual_address + main_image_base);
    // }

    // tls_id++;
    uint64_t ntdll_base_addr = get_base_address_from_peb(1);
    struct pe_optional_header64 *ntdll_opt_header64 = get_opt_header64(ntdll_base_addr);
    // if (ntdll_opt_header64->data_directories[IMAGE_DIRECTORY_ENTRY_TLS].virtual_address)
    // {
    // struct image_tls_directory64 *ntdll_tls_header64 = (struct data_directory *)(ntdll_opt_header64->data_directories[IMAGE_DIRECTORY_ENTRY_TLS].virtual_address + ntdll_base_addr);
    // process_tls(ntdll_tls_header64, &tls_id, pid, &top_addr, tls_id_buf);
    struct exception_directory *ntdll_exc_dir = (struct exception_directory *)(ntdll_opt_header64->data_directories[IMAGE_DIRECTORY_ENTRY_EXCEPTION].virtual_address + ntdll_base_addr);
    add_pdata_entry_when_load_image(ntdll_exc_dir, ntdll_base_addr, ntdll_opt_header64->size_of_image);
    // }
    pe_iat_process("ntdll.dll", &main_opt_header64->data_directories[IMAGE_DIRECTORY_ENTRY_IMPORT], ntdll_opt_header64, main_image_base, ntdll_base_addr);

    // tls_id++; // ninja.dll;

    struct data_directory import_dlls_directory = main_opt_header64->data_directories[IMAGE_DIRECTORY_ENTRY_IMPORT];
    struct pe_image_import_descriptor *import_descriptor = import_dlls_directory.virtual_address + main_image_base;
    while (1)
    {
        if (!import_descriptor || import_descriptor->name == 0)
            break;
        // Load the DLL
        uint64_t dll_base_memory_addr = top_addr;
        int64_t result = load_dll(import_descriptor->name + main_image_base, main_image_base, &top_addr, &import_dlls_directory, &tls_id, tls_id_buf);
        if (result < 0)
            break;
        // tls
        // struct image_tls_directory64 *dll_tls_dir = (struct image_tls_directory64 *)(get_opt_header64(dll_base_memory_addr)->data_directories[IMAGE_DIRECTORY_ENTRY_TLS].virtual_address + dll_base_memory_addr);
        // process_tls(dll_tls_dir, &tls_id, pid, &top_addr, tls_id_buf);
        // seh
        // struct exception_directory *dll_exc_dir = (struct exception_directory *)(get_opt_header64(dll_base_memory_addr)->data_directories[IMAGE_DIRECTORY_ENTRY_EXCEPTION].virtual_address + dll_base_memory_addr);
        // add_pdata_entry_when_load_image(dll_exc_dir, dll_base_memory_addr, get_opt_header64(dll_base_memory_addr)->size_of_image);

        import_descriptor++;
    }
    // iat forwarded import
    for (uint64_t o = 0; o < 3; o++)
    {
        // break;
        bool list_empty = true;
        for (uint64_t i = 0; i < sizeof(forwarded_imports) / sizeof(forwarded_imports[0]); i++)
        {
            if (forwarded_imports[i].import_addr == 0)
                continue;
            list_empty = false;
            char dll_full_name[256];
            char func_name[256];
            extract_dllname_and_funcname_in_forwarded_import_string(forwarded_imports[i].import_str, dll_full_name, func_name);
            strconcat(dll_full_name, dll_full_name, ".dll");
            int64_t forwarded_dll_id = find_dll_loaded(dll_full_name);
            int64_t loaded_dll_id = find_dll_loaded(dll_full_name);
            if (loaded_dll_id == -1)
            {
                load_dll(dll_full_name, main_image_base, &top_addr, &import_dlls_directory, &tls_id, tls_id_buf);
                loaded_dll_id = find_dll_loaded(dll_full_name);
            }
            resolve_forwarded_import(&forwarded_imports[i], loaded_dll_id, func_name);
        }
        if (list_empty)
            break;
    }
    // tls
    uint64_t i = 0;
    while (1)
    {
        uint64_t base = get_base_address_from_peb(i);
        if (!base)
            break;
        // run_crt(i);
        find_and_process_tls(i, &top_addr, tls_id_buf);
        i++;
    };
    // struct loaded_dll *debug1 = (uint64_t)peb + 4 * 4096;
    // run .crt
    // run all dllmain;
    // run_crt(0);
    i = 1;
    while (1)
    {
        uint64_t base = get_base_address_from_peb(i);
        if (i == 5)
        {
            // run_crt(i);
            run_dll_entry_point1(base);
            i++;
            continue;
        }
        if (!base)
            break;
        // run_crt(i);
        run_dll_entry_point(base);
        i++;
    };
    //================================
    i = 0;
    while (i<3)
    {
        uint64_t base = get_base_address_from_peb(i);
        uint64_t text_start = get_text_section(i)->virtual_address + base;
        if (!base)
            break;
        // run_crt(i);
        find_and_process_tls(i, &top_addr, tls_id_buf);
        i++;
    }
    crt_init_c();
};
// 0x1400013c9;