#ifndef EXE_COMMON_LIB_H
#define EXE_COMMON_LIB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#define IMAGE_ORDINAL_FLAG64 0x8000000000000000ULL
#define MAX_DLL_LIST_BLOCKS_COUNT 64
#define HEAP_START 0x100000000ULL
#define KUSER_SHARED_DATA 0x7FFE0000

struct dos_header_simplified
{
    uint16_t e_magic;  // 'MZ' = 0x5A4D
    uint8_t _pad[58];  
    uint32_t e_lfanew; 
} __attribute__((packed));
struct pe_header_simplified
{
    uint32_t signature; // 'PE\0\0' = 0x00004550
} __attribute__((packed));
struct pe_file_header
{
    uint16_t machine;
    uint16_t number_of_sections;
    uint32_t time_date_stamp;
    uint32_t pointer_to_symbol_table;
    uint32_t number_of_symbols;
    uint16_t size_of_optional_header;
    uint16_t characteristics;
} __attribute__((packed));
struct pe_image_import_descriptor
{
    uint32_t original_first_thunk; // RVA -> IMAGE_THUNK_DATA[]
    uint32_t time_date_stamp;
    uint32_t forwarder_chain;
    uint32_t name;        // RVA -> "KERNEL32.dll"
    uint32_t first_thunk; // RVA -> IAT
};
struct pe_optional_header32
{
    uint16_t magic; // 0x10B
    uint8_t major_linker_ver;
    uint8_t minor_linker_ver;

    uint32_t size_of_code;
    uint32_t size_of_init_data;
    uint32_t size_of_uninit_data;

    uint32_t address_of_entry; // RVA entry
    uint32_t base_of_code;
    uint32_t base_of_data;

    uint32_t image_base;
    uint32_t section_alignment;
    uint32_t file_alignment;

    uint32_t size_of_image;
    uint32_t size_of_headers;
} __attribute__((packed));

#define IMAGE_DIRECTORY_ENTRY_EXPORT 0
#define IMAGE_DIRECTORY_ENTRY_IMPORT 1
#define IMAGE_DIRECTORY_ENTRY_RESOURCE 2
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION 3
#define IMAGE_DIRECTORY_ENTRY_SECURITY 4
#define IMAGE_DIRECTORY_ENTRY_BASERELOC 5
#define IMAGE_DIRECTORY_ENTRY_DEBUG 6
#define IMAGE_DIRECTORY_ENTRY_ARCHITECTURE 7
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR 8
#define IMAGE_DIRECTORY_ENTRY_TLS 9
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG 10
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT 11
#define IMAGE_DIRECTORY_ENTRY_IAT 12
#define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT 13
#define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR 14
// 15 reserved
struct data_directory
{
    uint32_t virtual_address; // RVA
    uint32_t size;
};
struct pe_optional_header64
{
    uint16_t magic; // 0x20B
    uint8_t major_linker_ver;
    uint8_t minor_linker_ver;
    uint32_t size_of_code;
    uint32_t size_of_init_data;
    uint32_t size_of_uninit_data;
    uint32_t address_of_entry; // RVA entry
    uint32_t base_of_code;
    uint64_t image_base;
    uint32_t section_alignment;
    uint32_t file_alignment;
    uint16_t major_os_ver;        
    uint16_t minor_os_ver;        
    uint16_t major_image_ver;     
    uint16_t minor_image_ver;     
    uint16_t major_subsystem_ver; 
    uint16_t minor_subsystem_ver; 
    uint32_t win32_version_value; 
    uint32_t size_of_image;
    uint32_t size_of_headers;
    uint32_t checksum;
    uint16_t subsystem;
    uint16_t dll_characteristics;
    uint64_t size_of_stack_reserve;
    uint64_t size_of_stack_commit;
    uint64_t size_of_heap_reserve;
    uint64_t size_of_heap_commit;
    uint32_t loader_flags;
    uint32_t number_of_rva_and_sizes;
    struct data_directory data_directories[16];
} __attribute__((packed));
struct pe_section_header
{
    char name[8];                 // ".text"
    uint32_t virtual_size;        // 0x800
    uint32_t virtual_address;     // 0x1000
    uint32_t size_of_raw_data;    // 0x600
    uint32_t pointer_to_raw_data; // 0x400
    uint32_t _pad[3];
    uint32_t characteristics;
} __attribute__((packed));

struct pe_relocation_block
{
    uint32_t page_rva;
    uint32_t block_size;
} __attribute__((packed));

struct pe_export_directory
{
    uint32_t characteristics;
    uint32_t time_date_stamp;
    uint16_t major_version;
    uint16_t minor_version;
    uint32_t name;                     // RVA to DLL name
    uint32_t base;                     // starting ordinal number
    uint32_t number_of_functions;      // total number of functions
    uint32_t number_of_names;          // total number of names
    uint32_t address_of_functions;     // RVA to function addresses array
    uint32_t address_of_names;         // RVA to names array
    uint32_t address_of_name_ordinals; // RVA to ordinals array
} __attribute__((packed));
struct image_import_by_name
{
    uint16_t hint;
    char name[];
} __attribute__((packed));

///====================================

//====================================
#define UNI_REPLACEMENT_CHAR 0xFFFD

typedef struct _UNICODE_STRING
{
    uint16_t Length;        
    uint16_t MaximumLength; 
    uint16_t *Buffer;       
} UNICODE_STRING;
typedef struct _UNICODE_STRING32
{
    uint16_t len;     
    uint16_t max_len; 
    uint16_t *ptr;
    uint16_t buffer[32]; 
} ustr32;
typedef struct _UNICODE_STRING64
{
    uint16_t len;     
    uint16_t max_len; 
    uint16_t *ptr;
    uint16_t buffer[64]; 
} ustr64;
typedef struct _UNICODE_STRING256
{
    uint16_t len;     
    uint16_t max_len; 
    uint16_t *ptr;
    uint16_t buffer[256]; 
} ustr256;
typedef struct _UNICODE_STRING512
{
    uint16_t len;     
    uint16_t max_len; 
    uint16_t *ptr;
    uint16_t buffer[512]; 
} ustr512;
typedef struct _UNICODE_STRING1024
{
    uint16_t len;     
    uint16_t max_len; 
    uint16_t *ptr;
    uint16_t buffer[1024]; 
} ustr1024;
static size_t utf8_to_utf16(
    const char *in,
    size_t in_len,
    uint16_t *out,
    size_t out_len 
)
{
    size_t i = 0, o = 0;

    while (i <= in_len && o <= out_len)
    {
        uint32_t cp;
        uint8_t c = (uint8_t)in[i];

        if (c < 0x80)
        {
            cp = c;
            i += 1;
        }
        else if ((c & 0xE0) == 0xC0 && i + 1 < in_len)
        {
            cp = ((c & 0x1F) << 6) |
                 (in[i + 1] & 0x3F);
            i += 2;
        }
        else if ((c & 0xF0) == 0xE0 && i + 2 < in_len)
        {
            cp = ((c & 0x0F) << 12) |
                 ((in[i + 1] & 0x3F) << 6) |
                 (in[i + 2] & 0x3F);
            i += 3;
        }
        else if ((c & 0xF8) == 0xF0 && i + 3 < in_len)
        {
            cp = ((c & 0x07) << 18) |
                 ((in[i + 1] & 0x3F) << 12) |
                 ((in[i + 2] & 0x3F) << 6) |
                 (in[i + 3] & 0x3F);
            i += 4;
        }
        else
        {
            cp = UNI_REPLACEMENT_CHAR;
            i += 1;
        }

        if (cp <= 0xFFFF)
        {
            if (o + 1 > out_len)
                break;
            out[o++] = (uint16_t)cp;
        }
        else
        {
            if (o + 2 > out_len)
                break;
            cp -= 0x10000;
            out[o++] = 0xD800 | (cp >> 10);
            out[o++] = 0xDC00 | (cp & 0x3FF);
        }
    }

    return o; 
}
static size_t utf16_to_utf8(
    const uint16_t *in,
    size_t in_len,
    char *out,
    size_t out_len)
{
    size_t i = 0, o = 0;

    while (i <= in_len && o <= out_len)
    {
        uint32_t cp = in[i++];

        if (cp >= 0xD800 && cp <= 0xDBFF)
        {
            if (i < in_len)
            {
                uint32_t low = in[i];
                if (low >= 0xDC00 && low <= 0xDFFF)
                {
                    cp = ((cp - 0xD800) << 10) + (low - 0xDC00) + 0x10000;
                    i++;
                }
            }
        }

        if (cp < 0x80)
        {
            if (o + 1 > out_len)
                break;
            out[o++] = (char)cp;
        }
        else if (cp < 0x800)
        {
            if (o + 2 > out_len)
                break;
            out[o++] = 0xC0 | (cp >> 6);
            out[o++] = 0x80 | (cp & 0x3F);
        }
        else if (cp < 0x10000)
        {
            if (o + 3 > out_len)
                break;
            out[o++] = 0xE0 | (cp >> 12);
            out[o++] = 0x80 | ((cp >> 6) & 0x3F);
            out[o++] = 0x80 | (cp & 0x3F);
        }
        else
        {
            if (o + 4 > out_len)
                break;
            out[o++] = 0xF0 | (cp >> 18);
            out[o++] = 0x80 | ((cp >> 12) & 0x3F);
            out[o++] = 0x80 | ((cp >> 6) & 0x3F);
            out[o++] = 0x80 | (cp & 0x3F);
        }
    }

    return o; 
}
void utf8_to_ustr32(char *utf8_str, ustr32 *ustr)
{
    uint64_t len = strlen(utf8_str);
    size_t utf16_len = utf8_to_utf16(utf8_str, len, ustr->buffer, 32);
    ustr->len = len * 2;
    ustr->max_len = 32 * sizeof(uint16_t);
    ustr->ptr = &ustr->buffer;
}
void utf8_to_ustr64(char *utf8_str, ustr64 *ustr)
{
    uint64_t len = strlen(utf8_str);
    size_t utf16_len = utf8_to_utf16(utf8_str, len, ustr->buffer, 64);
    ustr->len = len * 2;
    ustr->max_len = 64 * sizeof(uint16_t);
    ustr->ptr = &ustr->buffer;
}
void utf8_to_ustr256(char *utf8_str, ustr256 *ustr)
{
    uint64_t len = strlen(utf8_str);
    size_t utf16_len = utf8_to_utf16(utf8_str, len, ustr->buffer, 256);
    ustr->len = len * 2;
    ustr->max_len = 256 * sizeof(uint16_t);
    ustr->ptr = &ustr->buffer;
}
void utf8_to_ustr512(char *utf8_str, ustr512 *ustr)
{
    uint64_t len = strlen(utf8_str);
    size_t utf16_len = utf8_to_utf16(utf8_str, len, ustr->buffer, 512);
    ustr->len = len * 2;
    ustr->max_len = 512 * sizeof(uint16_t);
    ustr->ptr = &ustr->buffer;
}
void utf8_to_ustr1024(char *utf8_str, ustr1024 *ustr)
{
    uint64_t len = strlen(utf8_str);
    size_t utf16_len = utf8_to_utf16(utf8_str, len, ustr->buffer, 1024);
    ustr->len = len * 2;
    ustr->max_len = 1024 * sizeof(uint16_t);
    ustr->ptr = &ustr->buffer;
}
void ustr_to_utf8(UNICODE_STRING *ustr, char *utf8_str, size_t max_utf8_len)
{
    size_t utf8_len = utf16_to_utf8((uint16_t *)ustr->Buffer, ustr->Length, NULL, 0);
    if (utf8_len == 0 || utf8_len >= max_utf8_len)
        return;

    utf16_to_utf8((uint16_t *)ustr->Buffer, ustr->Length, utf8_str, max_utf8_len);
    utf8_str[utf8_len] = '\0'; // null-terminate
}
//========================================================
void extract_file_name_from_path_utf16(char *path, ustr256 *file_name_u, size_t max_length)
{
    char *last_slash = 0;
    char file_name[256];
    for (char *p = path; *p != '\0'; p++)
    {
        if (*p == '\\' || *p == '/')
        {
            last_slash = p;
        }
    }
    if (last_slash)
    {
        memcpy(file_name, last_slash + 1, strlen(path) + 1);
    }
    else
    {
        memcpy(file_name, path, strlen(path) + 1);
    }
    utf8_to_ustr256(file_name, file_name_u);
};
uint64_t ustrlen(UNICODE_STRING *ustr)
{
    if (ustr == NULL || ustr->Buffer == NULL)
        return 0;
    return ustr->Length / sizeof(uint16_t);
};
void ustrconcat(UNICODE_STRING *dest, UNICODE_STRING *src1, UNICODE_STRING *src2)
{
    if (dest == NULL || src1 == NULL || src2 == NULL)
        return;

    size_t total_length = src1->Length + src2->Length;
    if (total_length > dest->MaximumLength)
        return;

    memcpy(dest->Buffer, src1->Buffer, src1->Length);
    memcpy(dest->Buffer + (src1->Length / sizeof(uint16_t)), src2->Buffer, src2->Length);
    dest->Length = total_length;
}
void ustrcpy_from_str(UNICODE_STRING *dst, char *src)
{
    size_t src_len = strlen(src);
    size_t utf16_len = utf8_to_utf16(src, src_len, (uint16_t *)dst->Buffer, dst->MaximumLength / sizeof(uint16_t));
    dst->Length = utf16_len * sizeof(uint16_t);
}
//========================================================
// main = image who load dll;
// sub:  dll being loaded;
struct process_params
{
    uint32_t MaximumLength; // 0x00
    uint32_t Length;        // 0x04

    uint32_t Flags;      // 0x08
    uint32_t DebugFlags; // 0x0C

    void *ConsoleHandle;   // 0x10
    uint32_t ConsoleFlags; // 0x18
    uint8_t Padding1[3];     // 0x1C

    void *StandardInput;  // 0x20
    void *StandardOutput; // 0x28
    void *StandardError;  // 0x30

    UNICODE_STRING CurrentDirectoryPath; // 0x38
    void *CurrentDirectoryHandle;        // 0x48

    UNICODE_STRING DllPath;       // 0x50
    UNICODE_STRING ImagePathName; // 0x60 ★
    UNICODE_STRING CommandLine;   // 0x70 ★

    void *Environment; // 0x80 ★ WCHAR**
};
#define MAX_ARGV_ENVP_CWD_LEN 4096
// struct RTL_CRITICAL_SECTION;
// struct RTL_CRITICAL_SECTION_DEBUG {
//     uint16_t Type;
//     uint16_t CreatorBackTraceIndex;
//     struct RTL_CRITICAL_SECTION *CriticalSection;
//     LIST_ENTRY ProcessLocksList;
//     uint32_t EntryCount;
//     uint32_t ContentionCount;
//     uint32_t Flags;
//     uint16_t CreatorBackTraceIndexHigh;
//     uint16_t SpareWORD;
// };
struct RTL_CRITICAL_SECTION
{
    
    void *DebugInfo;
    int32_t LockCount; // -1 = unlocked
    int32_t RecursionCount;
    uintptr_t OwningThread;  // TID
    uintptr_t LockSemaphore; 
    uintptr_t SpinCount;
};
struct PEB
{
    /* 0x000 */
    uint8_t InheritedAddressSpace;
    uint8_t ReadImageFileExecOptions;
    uint8_t BeingDebugged;
    uint8_t BitField;
    uint8_t Padding0[4]; 
    /* 0x008 */
    void *Mutant;
    void *ImageBaseAddress;
    /* 0x018 */
    struct list_loaded_dlls *loaded_dlls;  // pointer to PEB_LDR_DATA
    struct process_params *process_params; // RTL_USER_PROCESS_PARAMETERS*
    /* 0x028 */
    void *SubSystemData;
    void *ProcessHeap; 
    void *FastPebLock;

    /* 0x040 */
    void *AtlThunkSListPtr;
    void *IFEOKey;

    /* 0x050 */
    uint32_t CrossProcessFlags;
    uint32_t Padding;

    /* 0x058 */
    void *KernelCallbackTable; // ★ user32
    uint32_t SystemReserved;
    uint32_t AtlThunkSListPtr32;

    /* 0x068 */
    void *ApiSetMap; // ★ kernel32 / loader

    uint8_t padding0[8];
    /* 0x078 */
    void *TlsBitmap;         // PRTL_BITMAP
    uint32_t TlsBitmapBuffer[2];   // 64 slots

    /* 0x088 */
    // void *TlsExpansionBitmap;     // PRTL_BITMAP
    // uint32_t TlsExpansionBitmapBuffer[32]; // 1024 slots

    /* ================================================ */

    /* 0x110 */
    void *PostProcessInitRoutine;
    uint32_t SessionId;
    struct RTL_CRITICAL_SECTION peb_lock;
    struct process_params process_params_buf;
    uint8_t argv_envp_cwd_buf[MAX_ARGV_ENVP_CWD_LEN];
};
struct NT_TIB
{
    void *ExceptionList; 
    void *StackBase;     // high address
    void *StackLimit;    // low address
    void *SubSystemTib;
    void *FiberData;
    void *ArbitraryUserPointer;
    struct NT_TIB *Self; // points to itself
};
struct TEB
{
    /* 0x000 */ struct NT_TIB NtTib;

    /* 0x038 */ void *EnvironmentPointer;
    /* 0x040 */ void *ClientIdUniqueProcess;
    /* 0x048 */ void *ClientIdUniqueThread;

    /* 0x050 */ void *ActiveRpcHandle;
    /* 0x058 */ void *ThreadLocalStoragePointer;
    /* 0x060 */ struct PEB *ProcessEnvironmentBlock;

    /* 0x068 */ uint32_t LastErrorValue;
    /* 0x06C */ uint32_t CountOfOwnedCriticalSections;
    /* 0x070 */ void *CsrClientThread;
    /* 0x078 */ void *Win32ThreadInfo;

    /* 0x080 */ void *User32Reserved[26];
    /* 0x150 */ void *UserReserved[5];

    /* 0x178 */ void *WOW32Reserved;
    /* 0x180 */ uint32_t CurrentLocale;
    /* 0x184 */ uint32_t FpSoftwareStatusRegister;

    /* 0x188 */ void *SystemReserved1[54];
    /* 0x2F0 */ int ExceptionCode;

    /* TLS */
    /* 0x2F8 */ void *TlsSlots[64];
};
typedef struct _LIST_ENTRY
{
    struct _LIST_ENTRY *next; // pointer to next entry
    struct _LIST_ENTRY *prev; // pointer to previous entry
} LIST_ENTRY, *PLIST_ENTRY;
struct loaded_dll
{
    LIST_ENTRY InLoadOrderLinks;   
    LIST_ENTRY InMemoryOrderLinks; 
    LIST_ENTRY InInitializationOrderLinks;
    void *DllBase;              
    UNICODE_STRING FullDllName; 
    UNICODE_STRING BaseDllName; 
    char full_name[256];
    char name[32];
};
struct list_loaded_dlls
{
    uint32_t Length;
    uint8_t Initialized;
    void *SsHandle;
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
};
struct image_export_directory
{
    uint32_t Characteristics; 
    uint32_t TimeDateStamp;
    uint16_t MajorVersion;
    uint16_t MinorVersion;
    uint32_t Name;                  // RVA -> DLL name (char*)
    uint32_t Base;                  // starting ordinal number
    uint32_t NumberOfFunctions;     // size of AddressOfFunctions
    uint32_t NumberOfNames;         // size of AddressOfNames
    uint32_t AddressOfFunctions;    // RVA -> uint32_t[NumberOfFunctions]
    uint32_t AddressOfNames;        // RVA -> uint32_t[NumberOfNames]
    uint32_t AddressOfNameOrdinals; // RVA -> uint16_t[NumberOfNames]
};
struct kuser_shared_data
{
    uint8_t pad0[0x308];
    uint8_t SystemCallFlag; // +0x308
    uint8_t pad1[0x1000 - 0x309];
    uint64_t SystemCallDispatcher; // +0x1000;
};
uint64_t MAX_DLL_PER_PROCESS = MAX_DLL_LIST_BLOCKS_COUNT / sizeof(struct loaded_dll) * 4096;
uint64_t add_item_to_bottom_of_linked_list(LIST_ENTRY *list, LIST_ENTRY *item)
{
    if (list->next == 0)
    {
        list->next = item;
        list->prev = item;
        item->next = list;
        item->prev = list;
    }
    else
    {
        LIST_ENTRY *last = list->prev;
        last->next = item;
        item->prev = last;
        item->next = list;
        list->prev = item;
    }
};
uint64_t add_dll_to_bottom_of_linked_list(uint64_t new_dll_id, struct list_loaded_dlls *loaded_dlls_list, struct loaded_dll *loaded_dll)
{
    uint64_t peb_addr = 0;
    struct loaded_dll *newly_loaded_dll = &loaded_dll[new_dll_id];
    LIST_ENTRY *load_order_list = &loaded_dlls_list->InLoadOrderModuleList;
    LIST_ENTRY *memory_order_list = &loaded_dlls_list->InMemoryOrderModuleList;
    LIST_ENTRY *init_order_list = &loaded_dlls_list->InInitializationOrderModuleList;
    // InLoadOrderModuleList;
    add_item_to_bottom_of_linked_list(load_order_list, &newly_loaded_dll->InLoadOrderLinks);
    // InMemoryOrderModuleList;
    add_item_to_bottom_of_linked_list(memory_order_list, &newly_loaded_dll->InMemoryOrderLinks);
    // InInitializationOrderModuleList;
    add_item_to_bottom_of_linked_list(init_order_list, &newly_loaded_dll->InInitializationOrderLinks);
    return 0;
};
void setup_base_name_full_path(char *path, struct loaded_dll *loaded_dll)
{
    loaded_dll->FullDllName.Length = strlen(path);
    loaded_dll->FullDllName.MaximumLength = 256;
    loaded_dll->FullDllName.Buffer = (uint16_t *)(loaded_dll->full_name);
    ustr256 path_u;
    utf8_to_ustr256(path, &path_u);
    memcpy(loaded_dll->full_name, path_u.buffer, path_u.len + 2);
    ustr256 name;
    extract_file_name_from_path_utf16(path, &name, 32);
    memcpy(loaded_dll->name, name.buffer, name.len + 2);
    loaded_dll->BaseDllName.Length = name.len;
    loaded_dll->BaseDllName.MaximumLength = 64 / 2;
    loaded_dll->BaseDllName.Buffer = (uint16_t *)(loaded_dll->name);
};
void add_loaded_dll(uint64_t dll_id, char *path, struct list_loaded_dlls *loaded_dlls_list, struct loaded_dll *loaded_dll, uint64_t image_base_addr)
{
    struct loaded_dll *this_dll = &loaded_dll[dll_id];
    this_dll->DllBase = image_base_addr;
    setup_base_name_full_path(path, this_dll);
    // process ntdll info into dll list
    add_dll_to_bottom_of_linked_list(dll_id, loaded_dlls_list, loaded_dll);
}
bool extract_dllname_and_funcname_in_forwarded_import_string(char *import_str, char *dll_name, char *func_name)
{
    // Split the import string into DLL name and function name
    uint64_t delimiter_pos = 0;
    for (uint64_t i = 0; i < strlen(import_str); i++)
    {
        if (import_str[i] == '.')
        {
            delimiter_pos = i;
            break;
        }
    }
    if (!delimiter_pos)
        return false;
    memcpy(dll_name, import_str, delimiter_pos);
    dll_name[delimiter_pos] = '\0';
    memcpy(func_name, import_str + delimiter_pos + 1, strlen(import_str) - delimiter_pos - 1);
    func_name[strlen(import_str) - delimiter_pos - 1] = '\0';
    return true;
}
bool pe_image_relocation1(struct pe_optional_header64 *opt_header64, uint64_t main_image_base, uint64_t dll_base_memory_addr, int64_t offset_distance)
{
    struct data_directory reloc_directory = opt_header64->data_directories[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    if (reloc_directory.size == 0)
        return false; // no relocation info
    uint64_t reloc_table_base_addr = reloc_directory.virtual_address + dll_base_memory_addr;
    uint64_t reloc_section_byte_offset = 0;
    while (reloc_section_byte_offset < reloc_directory.size)
    {
        struct pe_relocation_block *reloc_block = (struct pe_relocation_block *)(reloc_table_base_addr + reloc_section_byte_offset);
        uint64_t entry_count = (reloc_block->block_size - sizeof(struct pe_relocation_block)) / sizeof(uint32_t);
        reloc_section_byte_offset += reloc_block->block_size;
        for (uint64_t i = 0; i < entry_count; i++)
        {
            uint64_t reloc_entry_addr = (uint64_t)reloc_block + 8 + i * 2;
            uint16_t entry = *(uint16_t *)reloc_entry_addr;
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
bool pe_thunk_is_ordinal(uint64_t thunk_rva)
{
    return (thunk_rva & IMAGE_ORDINAL_FLAG64) != 0;
}
uint64_t pe_find_export_real_address_by_ordinal(uint64_t ordinal_number, struct data_directory *sub_export_directory, uint64_t dll_base_memory_addr)
{
    // find the export table of the dll;
    struct pe_export_directory *export_table = (struct pe_export_directory *)(sub_export_directory->virtual_address + dll_base_memory_addr);
    uint64_t ordinal_index = ordinal_number - export_table->base;
    uint64_t max_export = export_table->number_of_names;
    uint64_t addr_of_functions_addr = export_table->address_of_functions + dll_base_memory_addr;
    uint64_t *functions_addr_ptr = (uint64_t *)addr_of_functions_addr;
    return functions_addr_ptr[ordinal_index] + dll_base_memory_addr;
};
int64_t pe_find_export_rva_by_name1(char *import_func_name, struct data_directory *sub_export_directory, uint64_t dll_base_memory_addr)
{
    // find the export table of the dll;
    struct pe_export_directory *export_table = (struct pe_export_directory *)(sub_export_directory->virtual_address + dll_base_memory_addr);
    uint64_t max_export = export_table->number_of_names;
    uint64_t i = 0;
    for (; i < max_export; i++)
    {
        uint32_t *name_rvas = (uint32_t *)(dll_base_memory_addr + export_table->address_of_names);
        char *export_name = (char *)(dll_base_memory_addr + name_rvas[i]);
        if (strcmp_i(export_name, import_func_name))
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
    int32_t function_rva = function_addresses[ordinal_index];
    // uint64_t function_real_address = dll_base_memory_addr + function_rva;
    return function_rva;
}
int64_t pe_find_main_import_descriptor(char *dll_file_name, struct data_directory *main_import_directory, struct pe_optional_header64 *sub_opt_header64, uint64_t main_image_base, uint64_t dll_base_memory_addr)
{
    uint64_t pe_image_import_descriptor_len = main_import_directory->size;
    if (pe_image_import_descriptor_len == 0)
        return -1;
    uint64_t import_id = 0;
    uint64_t fake_max_import_id = pe_image_import_descriptor_len / sizeof(struct pe_image_import_descriptor);
    // fake because actual max size is smaller, there are string buffer, paddings,...
    while (import_id < fake_max_import_id)
    {
        struct pe_image_import_descriptor *main_import_desc = (struct pe_image_import_descriptor *)(main_image_base + main_import_directory->virtual_address + import_id * sizeof(struct pe_image_import_descriptor));
        if (!main_import_desc->first_thunk && !main_import_desc->original_first_thunk)
            break;
        char *name = (char *)(main_image_base + main_import_desc->name);
        if (strcmp_iw(name, dll_file_name))
        {
            return import_id;
        }
        import_id++;
    }
    return -1;
};
int64_t pe_update_all_main_thunks(int64_t import_desc_id, struct data_directory *main_import_directory,
                                  struct data_directory *sub_export_directory, uint64_t main_image_base,
                                  uint64_t dll_base_memory_addr)
{
    struct pe_image_import_descriptor *main_import_desc = (struct pe_image_import_descriptor *)(main_image_base + main_import_directory->virtual_address + import_desc_id * sizeof(struct pe_image_import_descriptor));
    uint64_t first_thunk_ptr_addr = main_image_base + main_import_desc->first_thunk;
    uint64_t *thunk_rva_ptr = (uint64_t *)first_thunk_ptr_addr;
    uint64_t i = 0;
    while (thunk_rva_ptr[i])
    {
        if (pe_thunk_is_ordinal((uint64_t)thunk_rva_ptr[i]))
        {
            uint64_t ordinal_number = (uint64_t)thunk_rva_ptr[i] & (~IMAGE_ORDINAL_FLAG64);
            uint64_t corresponding_export = pe_find_export_real_address_by_ordinal(ordinal_number, sub_export_directory, dll_base_memory_addr);
            *(uint64_t *)&thunk_rva_ptr[i] = corresponding_export;
        }
        else
        {
            struct image_import_by_name *import = (struct image_import_by_name *)(thunk_rva_ptr[i] + main_image_base);
            char *thunk_name = (char *)import->name;
            int64_t corresponding_export = pe_find_export_rva_by_name1(thunk_name, sub_export_directory, dll_base_memory_addr);
            if (corresponding_export == 0)
            {
                echo("Cannot find function in imported dll");
                return -1;
            }
            if (corresponding_export < sub_export_directory->virtual_address || corresponding_export >= sub_export_directory->virtual_address + sub_export_directory->size)
            {
                thunk_rva_ptr[i] = corresponding_export + dll_base_memory_addr;
            }
            else
            {
                echo("Fwd Exp: ");
                echo(thunk_name);
                echo("\n");
                uint64_t fwd_imp_id = get_a_free_slot_with_size(forwarded_imports, 0, sizeof(forwarded_imports), sizeof(forwarded_imports[0]));
                if (fwd_imp_id == (uint64_t)-1)
                {
                    echo("No free slot for forwarded import\n");
                    return -1;
                }
                struct forwarded_import *fwd_imp = &forwarded_imports[fwd_imp_id];
                fwd_imp->import_addr = &thunk_rva_ptr[i];
                fwd_imp->import_str = (char *)corresponding_export + dll_base_memory_addr;
                thunk_rva_ptr[i] = 0x1234567890; 
            }
        }
        i++;
    };
};
uint64_t peb_addrx = 0;
struct loaded_dll *tttx = NULL;

uint64_t pe_iat_process(char *dll_file_name, struct data_directory *main_import_directory, struct pe_optional_header64 *sub_opt_header64,
                        uint64_t main_image_base, uint64_t dll_base_memory_addr)
{
    // return 0;
    int64_t import_desc_id = pe_find_main_import_descriptor(dll_file_name, main_import_directory, sub_opt_header64, main_image_base, dll_base_memory_addr);
    if (main_image_base == tttx[5].DllBase)
    {
        echo("Iating mscvrt\n");
    }
    if (import_desc_id < 0)
        return -1;
    struct data_directory *sub_export = &sub_opt_header64->data_directories[IMAGE_DIRECTORY_ENTRY_EXPORT];
    pe_update_all_main_thunks(import_desc_id, main_import_directory, sub_export, main_image_base, dll_base_memory_addr);
}
#endif // { EXE_LIB_H }
