#pragma once
#include <include/types.h>
#define PAGE_SIZE 4096
#define PAGE_PRESENT 0x1
#define PAGE_RW 0x2
#define PAGE_US 0x4
#define PAGE_PCD 0x10
#define PAGE_FLAGS PAGE_PRESENT | PAGE_RW | PAGE_PCD
#define TRANSLATION_UNKNOWN (void *) 0xFFFFFFFFFFFFFFFF

bool map_page(void *virtual_address, void *physical_address);
void *get_physical_address(void *virtual_addres);
static inline void flush_tlb_entry(void *old_virtual);