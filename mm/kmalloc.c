#include <mm/include/kmalloc.h>
#include <include/types.h>
#include <mm/include/memory_manager.h>
#include <mm/include/obj_alloc.h>
#include <include/mem.h>

void *__kheap = null; //base address of kernel's heap
uint64_t __kheap_final_size; //number of pages that the system could allocate for the heap
bool __kheap_ready = false;
pool_t __kheap_map_pool_id; //pool id of the descriptors
uint32_t __kheap_map_pool_ind; //index of the last descriptor in the pool + 1
uint8_t __kheap_ops; //number of operations on the heap's descriptors

/*
Initialize kernel heap memory and it's descriptors pool.
Fails if:
- the pool couldn't be created
- there's not enough memory to allocate the heap
- the first descriptor couldn't be pushed in the descriptors pool
*/
bool init_kmalloc(void) {
    //create the pool for the heap descriptors
    if (!create_obj_pool(&__kheap_map_pool_id, sizeof(__kheap_descriptor_t), __KHEAP_MAX_DESCRIPTORS, manual)) {
        return false;
    }

    //tries to allocate the heap with the default sizes (preferred, fallback and min)
    if ((__kheap = kalloc_page(__KHEAP_PREFERRED_SIZE_IN_PAGES)) != null) {
        __kheap_final_size = __KHEAP_PREFERRED_SIZE_IN_PAGES;
    } else if ((__kheap = kalloc_page(__KHEAP_FALLBACK_SIZE_IN_PAGES)) != null) {
        __kheap_final_size = __KHEAP_FALLBACK_SIZE_IN_PAGES;
    } else if ((__kheap = kalloc_page(__KHEAP_MIN_SIZE_IN_PAGES)) != null) {
        __kheap_final_size = __KHEAP_MIN_SIZE_IN_PAGES;
    } else {
        return false; //allocation failed, return false
    }

    __kheap_descriptor_t d;
    __kheap_init_descriptor(&d, null, __kheap_final_size * PAGE_SIZE, __KHEAP_DESCR_FLAGS_FREE);

    //tries to add the first descriptor, if the operation fails, free the heap and return false
    if (!obj_pool_put(__kheap_map_pool_id, (void *) &d, 0)) {
        kfree_page(__kheap);
        return false;
    }

    __kheap_map_pool_ind = 1;
    __kheap_ops = 0;
    __kheap_ready = true;
}

/*
Allocate a buffer in the kernel's heap.
This function scans the map to find a suitable memory space.
A memory space is suitable to be allocated if:
- is marked as free
- it's size is equal or greater than the size requested

Return the address of the buffer or null if the allocation fails.
The allocation fails if:
- a bad buffer size is requested
- the descriptors pool is full
- there was an error during an operation on the descriptors pool
- there's not enough memory in the heap
*/
void *kmalloc(uint64_t size) {
    if (size == 0 || size >= __kheap_final_size * PAGE_SIZE || !__kheap_ready) {return null;}
    if (__kheap_map_pool_ind == __KHEAP_MAX_DESCRIPTORS) {return null;} //if the pool is full, return null
    bool retried = false;

    retry:
    for (uint32_t i = 0; i < __kheap_map_pool_ind; i++) {
        __kheap_descriptor_t *entry = null;
        if (!obj_pool_get(__kheap_map_pool_id, (void **) &entry, i)) {return null;}
        if (!__KHEAP_DESCR_IS_FREE(entry->flags) || entry->size < size) {continue;} //if this descriptor is too small, skip it

        //create a new descriptor for the buffer and add it to the pool
        __kheap_descriptor_t new_entry;
        __kheap_init_descriptor(&new_entry, entry->base, size, __KHEAP_DESCR_FLAG_NULL);
        if (!obj_pool_put(__kheap_map_pool_id, (void *) &new_entry, __kheap_map_pool_ind)) {return null;}
        __kheap_map_pool_ind++;

        //edit the selected descriptor
        entry->size -= size;
        entry->base += size;

        //if this descriptor was the exact same size as required, delete it from the pool
        if (entry->size == 0) {
            memclear(entry, sizeof(__kheap_descriptor_t));
            if (!obj_pool_pack(__kheap_map_pool_id)) {return null;}
            __kheap_map_pool_ind--;
        }

        if (++__kheap_ops >= __KHEAP_NUM_OPS_BEFORE_COMPRESSION) {__kheap_compress_map();}
        return (void *)((uint64_t) __kheap + (uint64_t) new_entry.base);
    }

    if (!retried) {
        __kheap_compress_map();
        retried = true;
        goto retry;
    }

    return null;
}

/*
Realloc a buffer previously allocated with kmalloc().
*/
void *krealloc(void *base, uint64_t new_size) {
    if (base < __kheap || base >= __kheap + __kheap_final_size * PAGE_SIZE || !__kheap_ready || new_size >= __kheap_final_size * PAGE_SIZE) {
        return null;
    }

    for (uint32_t i = 0; i < __kheap_map_pool_ind; i++) {
        __kheap_descriptor_t *entry = null;
        if (!obj_pool_get(__kheap_map_pool_id, (void **) &entry, i)) {return null;}
        if (base < __kheap + (uint64_t) entry->base || base >= __kheap + (uint64_t) entry->base + entry->size) {continue;}
        uint64_t old_size = entry->size;
        uint8_t old_data[old_size];
        memmove(old_data, __kheap + (uint64_t) entry->base, old_size);
        kfree(base);
        void *new_buffer = kmalloc(new_size);
        if (!new_buffer) {return null;}
        memmove(new_buffer, old_data, old_size);
        return new_buffer;
    }

    return null;
}

/*
Free a buffer previously allocated with kmalloc().
*/
void kfree(void *base) {
    if (base < __kheap || base >= __kheap + __kheap_final_size * PAGE_SIZE || !__kheap_ready) {return;}

    for (uint32_t i = 0; i < __kheap_map_pool_ind; i++) {
        __kheap_descriptor_t *entry = null;
        if (!obj_pool_get(__kheap_map_pool_id, (void **) &entry, i)) {return;}
        if (base < __kheap + (uint64_t) entry->base || base >= __kheap + (uint64_t) entry->base + entry->size) {continue;}
        entry->flags |= __KHEAP_DESCR_FLAGS_FREE;
        if (++__kheap_ops >= __KHEAP_NUM_OPS_BEFORE_COMPRESSION) {__kheap_compress_map();}
        break;
    }
}

void __kheap_init_descriptor(__kheap_descriptor_t *d, void *base, uint64_t size, uint8_t flags) {
    d->base = base;
    d->size = size;
    d->flags = flags;
}

/*
Tries to compress the descriptors to make room in the descriptors pool.
This function is called automatically when the number of operations on the map is above a certain threshold value, in that case the map could be messy
and more descriptors could describe different parts of the same free memory area. Those descriptors can be merged into one.
*/
void __kheap_compress_map(void) {
    if (__kheap_map_pool_ind < 2 || !__kheap_ready) {return;}

    for (uint32_t i = 0; i < __kheap_map_pool_ind - 1; i++) {
        __kheap_descriptor_t *entry, *selected;
        if (!obj_pool_get(__kheap_map_pool_id, (void **) &entry, i)) {return;}
        selected = entry;

        for (uint32_t j = i + 1; j < __kheap_map_pool_ind; j++) {
            __kheap_descriptor_t *candidate;
            if (!obj_pool_get(__kheap_map_pool_id, (void **) &candidate, j)) {return;}

            if (candidate->base < selected->base) {
                selected = candidate;
            }
        }

        //if the selected entry is different from entry switch them
        if (selected != entry) {
            __kheap_descriptor_t tmp;
            memcpy((void *) &tmp, (void *) entry, sizeof(__kheap_descriptor_t));
            memmove((void *) entry, (void *) selected, sizeof(__kheap_descriptor_t));
            memmove((void *) selected, (void *) &tmp, sizeof(__kheap_descriptor_t));
        }

        //is the selected descriptor can be merged with the previous one merge them and resize the memory map
        if (i > 0 && (entry - 1)->base + (entry - 1)->size == selected->base && __KHEAP_DESCR_IS_FREE(selected->flags) && __KHEAP_DESCR_IS_FREE((entry - 1)->flags)) {
            (entry - 1)->size += selected->size;
            memclear(entry, sizeof(__kheap_descriptor_t));
            memmove((void *) entry, (void *)(entry + 1), (__kheap_map_pool_ind - i - 1) * sizeof(__kheap_descriptor_t));
            i--;
            __kheap_map_pool_ind--;
        }
    }
    
    __kheap_ops = 0; //reset the operations counter
}