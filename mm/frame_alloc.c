/*
This file contains variables and functions to allocate physical frames of memory.
These functions can be called AFTER initializing the physical allocator calling init_frame_alloc().
This allocator uses a bitmap to keep track of used frames, the bitmap is allocated and initialized by the boot loader, if the bit n in the bitmap is set
it means that the frame 4096 * n is free to use and can be allocated, 0 otherwise.
The bitmap covers only the physical memory available to the system.
This allocator uses a pool to pre-allocate some frames, the size of that pool (in frames) is defined in frame_allocator.h, when a frame is requested it takes
a frame from that pool. The pool is used like a stack, with a pointer to its top.
When the pool is empty and a frame is requested the allocator calls kalloc_pool() which attempts to fill it. If the pool can't be completely or partially
filled is because the system ran out of memory and the request for the frame is rejected. The search for free frames is linear but it uses an index to the
end of the last search and the index of the last free frame. The search is set to start from the last freed frame if its address is smaller than the saved one.
The pool can be in 3 states: fully_allocated, partially_allocated or empty, the state is altered by kalloc_frame() and kalloc_pool(),these states are defined
in frame_allocator.h.
When a frame is no longer needed it can be freed with kfree_frame().
*/

#include <include/types.h>
#include <mm/include/frame_alloc.h>
#include <mm/include/memory_manager.h>
#include <include/mem.h>

void *frame_bitmap = null;
uint32_t pool_top = 0; //index of the first available frame in the pool
pool_allocation_status pool_status = empty;
uint64_t pool[PREALLOC_POOL_SIZE]; //preallocated frames pool
uint32_t last_index = 0; //used to save the index of where the last search for a new frame ended so that the next search will start there
void *last_free_frame = null;
bool frame_alloc_ready = false;
extern uint64_t memory_length; //defined in memory_manager.c

bool init_frame_alloc(void *_frame_bitmap) {
    frame_bitmap = _frame_bitmap;
    memclear(pool, PREALLOC_POOL_SIZE * sizeof(uint64_t));
    pool_top = 0;
    pool_status = empty;
    last_index = 0;
    last_free_frame = null;
    frame_alloc_ready = true;
    kalloc_pool();
    return pool_status == fully_allocated || pool_status == partially_allocated;
}

/*
pre-allocate a pool of frames, it's called at startup or when the pool is empty. This function returns the new pool state.
The frames in the pool are not guaranteed to be contiguous.
*/
pool_allocation_status kalloc_pool(void) {
    if (!frame_alloc_ready) {
        return empty;
    }

    uint64_t n_frames = memory_length / PAGE_SIZE;

    for (uint64_t i = last_index; i < n_frames; i += 8) {
        uint8_t *byte = (uint8_t *)(frame_bitmap + i / 8); //get the byte no. i/8
        last_index = i;

        //iterate through the byte
        for (uint8_t j = 0; j < 8; j++) {
            if (*byte >> j & 1) {
                *byte -= 1 << j; //mark the page as used by setting the bit to 0
                pool[pool_top] = GET_ADDRESS_BY_OFFSET(i + j);
                last_free_frame = (void *) pool[pool_top];
                pool_top++;
            }

            if (pool_top == PREALLOC_POOL_SIZE) {
                pool_status = fully_allocated;
                pool_top--;
                return pool_status;
            }
        }
    }

    if (pool_top == 0) {
        pool_status = empty;
        last_index = 0;
    } else {
        pool_status = partially_allocated;
        pool_top--;
    }
    
    return pool_status;
}

void flush_frame_prealloc_pool(void) {
    for (uint32_t i = 0; i < PREALLOC_POOL_SIZE; i++) {
        kfree_frame((void *) pool[i]);
    }

    pool_top = 0;
    pool_status = empty;
}

//returns the first available frame and mark it as used
void *kalloc_frame(void) {
    if (!frame_alloc_ready) {
        return null;
    }

    //if the pool is empty call kalloc_pool to fill it and proceed
    if (pool_status == empty) {
        if (kalloc_pool() == empty) {
            return null; //if kalloc_pool couldn't allocate at least one page, we're out of memory
        }
    }

    //get the frame address
    uint64_t frame;

    if (pool_status == fully_allocated) {
        uint32_t index = PREALLOC_POOL_SIZE - 1 - pool_top;
        frame = pool[index];
        pool[index] = null;
    } else {
        frame = pool[pool_top];
        pool[pool_top] = null;
    }
    

    //if this was the last frame, set the pool status to empty so that the next time this function is called it will call kalloc_pool
    if (pool_top == 0) {
        pool_status = empty;
    } else {
        pool_top--; //otherwise set the new index
    }

    return (void *) frame;
}

//allocate a frame and clean it
void *kalloc_and_set_frame(void) {
    void *frame;

    if ((frame = kalloc_frame()) != null) {
        memclear(frame, PAGE_SIZE);
        return frame;
    }
    
    return null;
}

//free a frame in the memory map
bool kfree_frame(void *frame) {
    if (!frame_alloc_ready) {
        return false;
    }

    if ((uint64_t) frame >= memory_length) {
        return false;
    }

    uint64_t byte_n = GET_BYTE_FROM_ADDRESS((uint64_t) frame);
    uint8_t byte_offset = GET_BIT_FROM_ADDRESS((uint64_t) frame);
    *(uint8_t *)(frame_bitmap + byte_n) |= 1 << byte_offset;

    //if a frame has been freed, set last_search_index to 0 so that the next seatch will start from 0
    if (frame <= last_free_frame) {
        last_index = GET_BYTE_FROM_ADDRESS((uint64_t) frame);
        last_free_frame = frame;
    }
    
    return true;
}

bool kalloc_frames_array(uint32_t num_frames, void **array) {
    for (uint32_t i = 0; i < num_frames; i++) {
        if ((array[i] = kalloc_frame()) == null) {
            kfree_frames_array((i == 0 ? 0 : i - 1), array);
            return false;
        }
    }

    return true;
}

bool kfree_frames_array(uint32_t num_frames, void **array) {
    bool ret = true;

    for (uint32_t i = 0; i < num_frames; i++) {
        ret = ret && kfree_frame(array[i]);
    }

    return ret;
}

//Set a frame as used
void klock_frame(void *frame) {
    if (!frame_alloc_ready) {
        return;
    }

    if ((uint64_t) frame >= memory_length) {
        return;
    }

    uint64_t byte_n = GET_BYTE_FROM_ADDRESS((uint64_t) frame);
    uint8_t byte_offset = GET_BIT_FROM_ADDRESS((uint64_t) frame);
    *(uint8_t *)(frame_bitmap + byte_n) &= 0xFF - 1 << byte_offset;
}