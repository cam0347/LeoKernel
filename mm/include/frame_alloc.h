#pragma once
#include <include/types.h>
#define PREALLOC_POOL_SIZE 64
#define GET_BYTE_FROM_ADDRESS(addr) (((addr) / PAGE_SIZE) / 8)
#define GET_BIT_FROM_ADDRESS(addr) (((addr) / PAGE_SIZE) % 8)
#define GET_ADDRESS_BY_OFFSET(offset) ((offset) * PAGE_SIZE)

typedef enum {
    fully_allocated,
    partially_allocated,
    empty
} pool_allocation_status;

bool init_frame_alloc(void *);
bool kfree_frame(void *);
void *kalloc_frame(void);
void *kalloc_and_set_frame(void);
pool_allocation_status kalloc_pool(void);
void flush_frame_prealloc_pool(void);
void klock_frame(void *frame);
bool kalloc_frames_array(uint32_t num_frames, void **array);
bool kfree_frames_array(uint32_t num_frames, void **array);