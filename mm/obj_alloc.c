/*
Object pool allocator.
*/

#include <include/types.h>
#include <mm/include/obj_alloc.h>
#include <mm/include/frame_alloc.h>
#include <include/mem.h>
#include <mm/include/memory_manager.h>

object_pool_descriptor_t obj_pools[OBJECT_POOL_MAX_POOLS];
//leokernel_memory_descriptor_t mmap_pool_memory[MMAP_POOL_MEMORY_SIZE];
uint8_t mmap_pool_memory[MMAP_POOL_MEMORY_SIZE];
uint32_t obj_pools_index = 0;

bool obj_alloc_init(pool_t *id) {
    memclear(obj_pools, OBJECT_POOL_MAX_POOLS * sizeof(object_pool_descriptor_t));
    memclear(mmap_pool_memory, MMAP_POOL_MEMORY_SIZE);

    //create the pool that will contain the memory map, this can't be created with create_obj_pool()
    init_first_pool((void *) mmap_pool_memory);
    *id = 0;

    obj_pools_index = 1;
    return true;
}

//function that initialize the first pool which will contain the memory map
void init_first_pool(void *base) {
    obj_pools[0].base = base;
    obj_pools[0].obj_size = sizeof(leokernel_memory_descriptor_t);
    obj_pools[0].pool_size = MMAP_POOL_MEMORY_SIZE / sizeof(leokernel_memory_descriptor_t);
    obj_pools[0].id = 0;
    obj_pools[0].type = manual;
    obj_pools[0].top_index = 0;
    obj_pools[0].bottom_index = 0;
}

bool create_obj_pool(pool_t *id, uint32_t obj_size, uint32_t pool_size, object_pool_type type) {
    //if the pool descriptors array is full return false
    if (obj_pools_index == OBJECT_POOL_MAX_POOLS) {
        return false; //the pool can't be created
    }

    uint64_t total_size = obj_size * pool_size;
    uint64_t pages = total_size / PAGE_SIZE + total_size % PAGE_SIZE != 0 ? 1 : 0; //calculate the number of pages to allocate for this pool
    void *base = kalloc_page(pages);

    if (!base) {
        return false;
    }

    memclear(base, pages * PAGE_SIZE);
    object_pool_descriptor_t *entry = &obj_pools[obj_pools_index];
    entry->base = base;
    entry->obj_size = obj_size;
    entry->pool_size = pool_size;
    entry->id = obj_pools_index;
    entry->bottom_index = 0;
    entry->top_index = 0;
    entry->type = type;
    obj_pools_index++;
    *id = entry->id;
    return true;
}

/*
Puts an object into a pool.
This function copy the object without erasing it from its original location.
Return true if the insertion went good, false if: a bad pool id or object index is provided or if the pool is full.
If this is not a manual pool, the object index field is ignored.
*/
bool obj_pool_put(pool_t id, void *obj, uint32_t obj_index) {
    //checks if the pool id is valid (it doesn't exceed pools array capacity)
    if (id >= obj_pools_index) {
        return false; //illegal pool id
    }

    object_pool_descriptor_t *pool = &obj_pools[id]; //get the pool descriptor

    //checks if the object index is valid (it doesn't exceed the pools capacity)
    if (obj_index >= pool->pool_size && pool->type == manual) {
        return false; //illegal object index
    }

    //if this pool is manual copy the object into the pool at the specified index and return
    if (pool->type == manual) {
        memcpy(pool->base + pool->obj_size * obj_index, obj, pool->obj_size);
    } else if (pool->type == stack) {
        //if this pool is full return false
        if (pool->top_index == pool->pool_size) {
            return false;
        }

        memcpy(pool->base + pool->obj_size * pool->top_index, obj, pool->obj_size);
        pool->top_index++;
    } else if (pool->type == queue) {
        if (pool->top_index < pool->pool_size) {
            memcpy(pool->base + pool->obj_size * pool->top_index, obj, pool->obj_size);
            pool->top_index++;
        } else if (pool->bottom_index > 0) {
            memmove(pool->base, pool->base + pool->obj_size * pool->bottom_index, pool->obj_size * (pool->top_index - pool->bottom_index));
            pool->top_index -= pool->bottom_index;
            pool->bottom_index = 0;
            memcpy(pool->base + pool->obj_size * pool->top_index, obj, pool->obj_size);
            pool->top_index++;
        } else {
            return false;
        }
    } else {
        return false; //unknown pool type, return false
    }

    return true;
}

/*
Gets an object from a pool.
This function sets a pointer obj (parameter) to the right object. It's not a good idea to use that pointer to access other objects.
Note that if the pool is a stack or a queue and an object is "removed", the object still resides in the pool memory, it's just no longer accessible
with this function because the pool internal pointers are changed.
Return true if the insertion went good, false if: a bad pool id or object index is provided or if the pool is empty.
If this is not a manual pool, the object index field is ignored.
*/
bool obj_pool_get(pool_t id, void **obj, uint32_t obj_index) {
    *obj = null;
    
    //checks if the pool id is valid (it doesn't exceed pools array capacity)
    if (id >= obj_pools_index) {
        return false; //illegal pool id
    }

    object_pool_descriptor_t *pool = &obj_pools[id]; //get the pool descriptor

    //checks if the object index is valid (it doesn't exceed the pool's capacity)
    if (obj_index >= pool->pool_size && pool->type == manual) {
        return false; //illegal object index
    }

    if (pool->type == manual) {
        *obj = pool->base + pool->obj_size * obj_index;
    } else if (pool->type == stack) {
        //if the stack is empty return false
        if (pool->top_index == 0) {
            return false;
        }

        *obj = pool->base + pool->obj_size * (pool->top_index - 1);
        pool->top_index--;
    } else if (pool->type == queue) {
        //if the queue is empty return false
        if (pool->bottom_index == pool->pool_size) {
            return false;
        }

        *obj = pool->base + pool->obj_size * pool->bottom_index;
        pool->bottom_index++;
    } else {
        return false;
    }

    return true;
}

//flushes a pool (clear it's memory)
bool obj_pool_flush(pool_t id) {
    //checks if the pool id is valid (it doesn't exceed pools array capacity)
    if (id >= obj_pools_index) {
        return false; //illegal pool id
    }

    object_pool_descriptor_t *pool = &obj_pools[id]; //get the pool descriptor
    memclear(pool->base, pool->obj_size * pool->pool_size);
    pool->top_index = 0;
    pool->bottom_index = 0;
    return true;
}

//change pool type, if needed compact the objects, this should be avoided because this is requires a heavy computation running in O(n^2)
bool obj_pool_change_type(pool_t id, object_pool_type new_type) {
    //checks if the pool id is valid (it doesn't exceed pools array capacity)
    if (id >= obj_pools_index) {
        return false; //illegal pool id
    }

    object_pool_descriptor_t *pool = &obj_pools[id]; //get the pool descriptor
    pool->type = new_type;

    if (new_type == manual) {
        pool->bottom_index = 0;
        pool->top_index = 0;
        return true;
    } else if (new_type == stack || new_type == queue) {
        return obj_pool_pack(id);
    } else {
        return false; //invalid new type
    }
}

//compacts a pool, this should be avoided because it runs in O(n^2) time
bool obj_pool_pack(pool_t id) {
    //checks if the pool id is valid (it doesn't exceed pools array capacity)
    if (id >= obj_pools_index) {
        return false; //illegal pool id
    }

    object_pool_descriptor_t *pool = &obj_pools[id]; //get the pool descriptor

    //this function can be called only on manual and queue pools
    if (pool->type == stack) {
        return false;
    }

    //delete those unaccessible objects before executing the algorithm
    if (pool->type == queue) {
        memclear(pool->base, pool->obj_size * pool->bottom_index);
        memclear(pool->base + pool->obj_size * pool->top_index, pool->obj_size * (pool->pool_size - pool->top_index));
    }

    uint8_t empty_obj[pool->obj_size];
    memclear(empty_obj, pool->obj_size);
    pool->bottom_index = 0;

    for (uint32_t i = 0; i < pool->pool_size - 1; i++) {
        void *obj = pool->base + pool->obj_size * i;

        if (!memcmp(obj, empty_obj, pool->obj_size)) {
            continue;
        }

        bool found = false;

        for (uint32_t j = i + 1; j < pool->pool_size; j++) {
            void *candidate = pool->base + pool->obj_size * j;

            if (!memcmp(candidate, empty_obj, pool->obj_size)) {
                memmove(obj, candidate, pool->obj_size);
                found = true;
                pool->top_index = i + 1;
                i--;
                break;
            }
        }

        if (!found) {
            break;
        }
    }

    return true;
}