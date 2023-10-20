#pragma once
#include <include/types.h>
#include <mm/include/frame_alloc.h>
#define PAGE_SIZE 4096
#define ALLOC_MAX_PAGES 0x8000000
#define LEOKERNEL_MEMORY_MAP_NULL_FLAGS 0
#define LEOKERNEL_MEMORY_MAP_AVAIL 1 << 0
#define LEOKERNEL_MEMORY_MAP_VALID 1 << 1
#define LEOKERNEL_MEMORY_MAP_TRANS_UNK 1 << 2
#define LEOKERNEL_MEMORY_MAP_HAS_NEXT 1 << 3
#define DESCRIPTOR_AVAILABLE(flags) (flags & 1)
#define DESCRIPTOR_VALID(flags) (flags >> 1 & 1)
#define DESCRIPTOR_TRANS_UNK(flags) (flags >> 2 & 1)
#define DESCRIPTOR_HAS_NEXT(flags) (flags >> 3 & 1)
struct leokernel_boot_params;

/*
flags (bits):
0: available, this memory is free to be used
1: valid, used to know when the descriptor list is finished, when an invalid descriptor is found, the end is reached
2: translation unknown, set when the physical address is unknown
3: has next, set when the descriptor after is a continuation of this

type (value):
0: usable
1: kernel reserved
2: acpi reserved
3: firmware reserved
4: I/O reserved
5: other reserved
*/
typedef struct {
    uint64_t virtual_address;
    uint64_t physical_address;
    uint32_t pages;
    uint8_t flags;
    uint8_t type;
    uint8_t pad[10];
} leokernel_memory_descriptor_t;

enum leokernel_memory_descriptor_type {
    usable,
    kernel_reserved,
    acpi_reserved,
    firmware_reserved,
    io_reserved,
    other_reserved
};

bool init_mm(struct leokernel_boot_params boot_parameters);
pool_allocation_status kalloc_pool(void);
void *kalloc_frame(void);
bool kfree_frame(void *frame);
void *kalloc_page(uint32_t pages);
bool kfree_page(void *base);
leokernel_memory_descriptor_t *find_available_virtual_memory(uint32_t num_pages, uint64_t *descr_ind);
void init_descriptor(leokernel_memory_descriptor_t *descr, void *virtual_address, void *physical_address, uint32_t pages, uint8_t flags, uint8_t type);
void order_map(leokernel_memory_descriptor_t *map, uint64_t size);
void compress_map(leokernel_memory_descriptor_t *map, uint64_t size);
bool kmap_page(void *virtual, void *physical);