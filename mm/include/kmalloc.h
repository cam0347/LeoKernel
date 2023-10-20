#pragma once
#include <include/types.h>
#define __KHEAP_MAX_DESCRIPTORS 128
#define __KHEAP_PREFERRED_SIZE_IN_PAGES 256 //preferred number of pages to allocate for the kernel's heap
#define __KHEAP_FALLBACK_SIZE_IN_PAGES 128  //fallback number of pages to allocate for the kernel's heap
#define __KHEAP_MIN_SIZE_IN_PAGES 64        //minimum size of pages to allocate for the kernel's heap
#define __KHEAP_DESCR_FLAG_NULL 0
#define __KHEAP_DESCR_FLAGS_FREE 1 << 0
#define __KHEAP_DESCR_IS_FREE(flags) (flags & 1)
#define __KHEAP_NUM_OPS_BEFORE_COMPRESSION 32 //number of operations on the heap (alloc/free) before compressing the map, the compression could happen even if the descriptor pool is full

typedef struct {
    void *base;
    uint64_t size;
    uint8_t flags;
    uint8_t pad[15];
} __kheap_descriptor_t;

bool init_kmalloc(void);
void __kheap_init_descriptor(__kheap_descriptor_t *d, void *base, uint64_t size, uint8_t flags);
void *kmalloc(uint64_t buffer_size);
void *krealloc(void *base, uint64_t new_size);
void kfree(void *base);
void __kheap_compress_map(void);