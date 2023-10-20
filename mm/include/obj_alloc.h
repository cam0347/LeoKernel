#pragma once
#include <include/types.h>
#define OBJECT_POOL_MAX_POOLS 32
#define MMAP_POOL_MEMORY_SIZE PAGE_SIZE
typedef uint8_t pool_t;

//type of pool (aka how the pool is managed)
typedef enum {
    manual, //the client use it's own index to access the pool
    stack,  //the client uses obj_pool_get() and obj_pool_put() to get and insert an object into the pool
    queue   //same as stack
} object_pool_type;

typedef struct {
    void *base; //base address of the pool
    uint32_t obj_size; //size of the object
    uint32_t pool_size; //size of the pool in objects
    pool_t id;
    uint32_t top_index, bottom_index;
    object_pool_type type;
} object_pool_descriptor_t;

bool obj_alloc_init(pool_t *id_ptr);
void init_first_pool(void *base);
bool create_obj_pool(pool_t *id_ptr, uint32_t obj_size, uint32_t pool_size, object_pool_type type);
bool obj_pool_put(pool_t id, void *obj, uint32_t index);
bool obj_pool_get(pool_t id, void **buffer, uint32_t index);
bool obj_pool_pack(pool_t id);