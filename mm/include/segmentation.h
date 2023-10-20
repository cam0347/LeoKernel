#pragma once
#include <include/types.h>
#define GDT_MAX_ENTRIES 8
#define GDT_NULL_ENTRY 0
#define GDT_KERNEL_CS 1
#define GDT_KERNEL_DS 2
#define GDT_USER_CS 3
#define GDT_USER_DS 4
#define GDT_TSS 5

typedef struct {
    /*uint16_t limit_0; //0-15
    uint16_t base_0; //0-15
    uint8_t base_1; //16-23
    uint8_t access;
    uint8_t limit_1:4; //16-19
    uint8_t flags:4;
    uint8_t base_2; //24-31
    uint32_t base_3; //32-63
    uint32_t reserved;*/
    uint16_t limit_0;
    uint16_t base_0;
    uint8_t base_1;
    uint8_t access;
    uint8_t limit_1:4;
    uint8_t flags:4;
    uint8_t base_2;
} __attribute__((packed)) gdt_t;

typedef struct {
    uint16_t size;
    uint64_t base;
} __attribute((packed)) gdtr_t;

bool setup_gdt();
bool load_gdt(uint32_t, uint64_t, uint32_t, uint8_t, uint8_t);
void gdt_load_segments();