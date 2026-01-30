#include <stdint.h>
#include <stddef.h>

#define MULTIBOOT_TAG_TYPE_MMAP 6

extern uint64_t total_memory;
extern uint64_t cores_count;

extern uint64_t total_memory_blocks_count;
typedef struct {
    uint32_t type;
    uint32_t size;
} multiboot_tag;

typedef struct {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
} __attribute__((packed)) multiboot_mmap_entry;

typedef struct {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    multiboot_mmap_entry entries[];
} __attribute__((packed)) multiboot_tag_mmap;

extern void* multiboot_info_ptr_64;

void get_total_memory() {
    uint8_t* ptr = (uint8_t*)multiboot_info_ptr_64;
    multiboot_tag* tag = (multiboot_tag*)(ptr + 8);  // skip total_size + reserved
    uint64_t total = 0;

    while (tag->type != 0) {
        if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) {
            multiboot_tag_mmap* mmap = (multiboot_tag_mmap*)tag;
            uint8_t* entry_ptr = (uint8_t*)mmap->entries;
            uint8_t* end = (uint8_t*)tag + tag->size;

            while (entry_ptr < end) {
                multiboot_mmap_entry* entry = (multiboot_mmap_entry*)entry_ptr;
                if (entry->type == 1) {
                    total += entry->length;
                }
                entry_ptr += mmap->entry_size;
            }
            break;
        }

        tag = (multiboot_tag*)(((uint64_t)tag + tag->size + 7) & ~7);  // align 8
    }

    if (total > 128ULL*1024*1024*1024*1024)
        total = 128ULL*1024*1024*1024*1024; // max 128TB;
    total_memory_blocks_count = total / 4096;
    total_memory = total_memory_blocks_count*4096; // round down to blocks 4kb;
}
