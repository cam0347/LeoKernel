#include <mm/include/segmentation.h>
#include <include/assert.h>
#include <include/low_level.h>

gdt_t gdt[GDT_MAX_ENTRIES] __attribute__((aligned(sizeof(gdt_t))));
gdtr_t gdtr;

//loads basic segment descriptors and load the gdtr
bool setup_gdt() {
    assert_true(load_gdt(GDT_NULL_ENTRY, 0, 0, 0, 0)); //null descriptor
    assert_true(load_gdt(GDT_KERNEL_CS, 0, 0xFFFFF, 0x9A, 0xA)); //kernel mode code segment
    assert_true(load_gdt(GDT_KERNEL_DS, 0, 0xFFFFF, 0x92, 0xA)); //kernel mode data segment
    assert_true(load_gdt(GDT_USER_CS, 0, 0xFFFFF, 0xFA, 0xA)); //user mode code segment
    assert_true(load_gdt(GDT_USER_DS, 0, 0xFFFFF, 0xF2, 0xA)); //user mdoe data segment
    //assert_true(load_gdt(GDT_TSS, 0/*TSS base*/, 0/*size of TSS*/, 0x89, 0x0)); //TSS
    gdtr.base = (uint64_t) gdt;
    gdtr.size = 5 * sizeof(gdt_t) - 1;
    disable_int();
    asm volatile("lgdt %0"::"m"(gdtr));
    enable_int();
    return true;
}

//sets an entry of the gdt
bool load_gdt(uint32_t n, uint64_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    if (n >= GDT_MAX_ENTRIES) {
        return false;
    }

    gdt_t *entry = &gdt[n];
    entry->limit_0 = limit & 0xFFFF; //ignored
    entry->base_0 = base & 0xFFFF; //ignored
    entry->base_1 = base >> 16 & 0xFF; //ignored
    entry->access = access;
    entry->limit_1 = limit >> 16 & 0x0F; //ignored
    entry->flags = flags & 0x0F;
    entry->base_2 = base >> 24 & 0xFF; //ignored

    return true;
}

//loads the new segment selectors into cpu's registers
void gdt_load_segments() {
    asm volatile(".intel_syntax");
    asm volatile("int 0x16");
    asm volatile(".att_syntax");
}