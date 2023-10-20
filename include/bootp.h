#pragma once
#include <include/types.h>
#include <mm/include/memory_manager.h>

struct leokernel_boot_params {
    char *frame_buffer;
    uint64_t frame_buffer_size;
    uint64_t video_height;
    uint64_t video_width;
    uint64_t video_pitch;
    void *font;
    leokernel_memory_descriptor_t *map;
    uint64_t map_size;
    void *frame_bitmap;
    void *xsdt;
};