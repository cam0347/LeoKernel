#include <mm/include/paging.h>
#include <include/types.h>
#include <include/low_level.h>
#include <mm/include/memory_manager.h>

bool map_page(void *virtual, void *physical) {
    uint64_t _virtual = (uint64_t) virtual;
    uint16_t field0 = (_virtual >> 39) & 0x1FF; //PML4 index (39-48)
    uint16_t field1 = (_virtual >> 30) & 0x1FF; //pdpt index (30-38)
    uint16_t field2 = (_virtual >> 21) & 0x1FF; //page directory index (21-29)
    uint16_t field3 = (_virtual >> 12) & 0x1FF; //page table index (12-20)
    uint64_t *pml4 = (uint64_t *)(get_cr3() & 0xFFFFFFFFFFFFF000);
    uint64_t *pdpt, *pd, *pt;

    if (pml4[field0] == null) {
        void *new_entry;
        if ((new_entry = kalloc_frame()) == null) {
            return false;
        }

        pml4[field0] = (uint64_t) new_entry | PAGE_FLAGS;
        pdpt = (uint64_t *) new_entry;
    } else {
        pdpt = (uint64_t *)(pml4[field0] & 0xFFFFFFFFFFFFF000);
    }

    if (pdpt[field1] == null) {
        void *new_entry;
        if ((new_entry = kalloc_frame()) == null) {
            return false;
        }

        pdpt[field1] = (uint64_t) new_entry | PAGE_FLAGS;
        pd = (uint64_t *) new_entry;
    } else {
        pd = (uint64_t *)(pdpt[field1] & 0xFFFFFFFFFFFFF000);
    }

    if (pd[field2] == null) {
        void *new_entry;
        if ((new_entry = kalloc_frame()) == null) {
            return false;
        }

        pd[field2] = (uint64_t) new_entry | PAGE_FLAGS;
        pt = (uint64_t *) new_entry;
    } else {
        pt = (uint64_t *)(pd[field2] & 0xFFFFFFFFFFFFF000);
    }

    flush_tlb_entry(virtual);
    pt[field3] = (uint64_t) physical | PAGE_FLAGS;
    return true;
}

void *get_physical_address(void *virtual) {
    uint64_t _virtual = (uint64_t) virtual;
    uint16_t field0 = (_virtual >> 39) & 0x1FF; //bits 39-48
    uint16_t field1 = (_virtual >> 30) & 0x1FF; //bits 30-38
    uint16_t field2 = (_virtual >> 21) & 0x1FF; //bits 21-29
    uint16_t field3 = (_virtual >> 12) & 0x1FF; //bits 12-20
    uint16_t offset = _virtual & 0xFFF;
    uint64_t *pml4 = (uint64_t *)(get_cr3() & 0xFFFFFFFFFFFFF000);
    uint64_t *pdpt, *pd, *pt;

    if (pml4[field0] == null) {
        return TRANSLATION_UNKNOWN;
    } else {
        pdpt = (uint64_t *)(pml4[field0] & 0xFFFFFFFFFFFFF000);
    }

    if (pdpt[field1] == null) {
        return TRANSLATION_UNKNOWN;
    } else {
        pd = (uint64_t *)(pdpt[field1] & 0xFFFFFFFFFFFFF000);
    }

    if (pd[field2] == null) {
        return TRANSLATION_UNKNOWN;
    } else {
        pt = (uint64_t *)(pd[field2] & 0xFFFFFFFFFFFFF000);
    }

    return  (void *)(pt[field3] & 0xFFFFFFFFFFFFF000 | offset);
}

static inline void flush_tlb_entry(void *addr) {
    asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}
