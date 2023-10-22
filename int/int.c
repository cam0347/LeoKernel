#include <int/include/int.h>
#include <mm/include/segmentation.h>
#include <include/low_level.h>
#include <int/include/apic.h>
#include <io/include/port_io.h>
#include <mm/include/obj_alloc.h>
#include <include/mem.h>

/*
0x00 	Division by zero
0x01 	Single-step interrupt
0x02 	NMI
0x03 	Breakpoint
0x04 	Overflow
0x05 	Bound Range Exceeded
0x06 	Invalid Opcode
0x07 	Coprocessor not available
0x08 	Double Fault
0x09 	Coprocessor Segment Overrun (386 or earlier only)
0x0A 	Invalid Task State Segment
0x0B 	Segment not present
0x0C 	Stack Segment Fault
0x0D 	General Protection Fault
0x0E 	Page Fault
0x0F 	reserved
0x10 	x87 Floating Point Exception
0x11 	Alignment Check
0x12 	Machine Check
0x13 	SIMD Floating-Point Exception
0x14 	Virtualization Exception
0x15 	Control Protection Exception
*/

__attribute__((aligned(sizeof(idt_t)))) //16 byte aligned
idt_t idt[MAX_INTERRUPT]; //vector for ISRs addresses
idtr_t idtr; //idt register structure
extern uint64_t isrs[];
uint8_t gsi_map[256];
void *isr_hooks[256][ISR_MAX_HOOKS];
bool nmi_enabled = true;

//loads idt with 256 entries and loads idtr
bool setup_interrupts() {
    uint16_t limit = 0; //used to count how many isr are available

    for (uint8_t i = 0; i < MAX_INTERRUPT - 1; i++) {
        if (!isrs[i]) {
            break;
        }
        
        set_idt_entry(i, isrs[i], ISR_FLAG_PRESENT | ISR_FLAG_RING0 | ISR_TYPE_INT);
        limit++;
    }

    idtr.base = (uint64_t) idt;
    idtr.limit = limit * sizeof(idt_t) - 1;
    disable_int();
    asm volatile("lidt %0" : : "m"(idtr));
    enable_int(); //enables maskable interrupts
    nmi_enable(); //enables non-maskable interrupts
    
    for (uint32_t i = 0; i < 256; i++) {
        gsi_map[i] = i;
    }

    memclear(isr_hooks, 256 * ISR_MAX_HOOKS * sizeof(void *));
    return true;
}

//sets a specific entry into the idt
void set_idt_entry(uint8_t n, uint64_t isr, uint8_t flags) {
    idt_t *entry = &idt[n];
    entry->isr_0 = isr & 0xFFFF;
    entry->isr_1 = isr >> 16 & 0xFFFF;
    entry->isr_2 = isr >> 32 & 0xFFFFFFFF;
    entry->kernel_cs = GDT_KERNEL_CS * sizeof(gdt_t);
    entry->ist = 0;
    entry->flags = flags;
    entry->reserved = 0;
}

void nmi_enable() {
    outb(NMI_CONTROL_REGISTER, inb(NMI_CONTROL_REGISTER) & 0x7F); //bit 7 unset -> nmi enabled
    inb(0x71);
    nmi_enabled = true;
}
 
void nmi_disable() {
    outb(NMI_CONTROL_REGISTER, inb(NMI_CONTROL_REGISTER) | 0x80); //bit 7 set -> nmi disabled
    inb(0x71);
    nmi_enabled = false;
}

/* interrupt hook functions */

/*
hooks an interrupt to a handler function.
arguments:
- int_n: interrupt number (0-255)
- handler: pointer to the handler function

returns:
- 0xFF if a bad argument is passed or if a interrupt has all its hooks used
- n < ISR_MAX_HOOKS if the handler is succesfully hooked to the interrupt, n is the index of the hook, use that number to unhook it
*/
uint8_t int_hook(uint8_t int_n, void *handler) {
    if (!handler) {
        return 0xFF;
    }

    uint8_t i = 0;

    while(i < ISR_MAX_HOOKS && isr_hooks[int_n][i]) {
        i++;
    }

    if (i == ISR_MAX_HOOKS) {
        return 0xFF;
    }

    isr_hooks[int_n][i] = handler;
    return i;
}

/*
unhooks a function handler from an interrupt.
this function sets to null the hook attached to an interrupt with int_hook().
arguments:
- int_n: interrupt number (0-255)
- hook_n: hook identifier previously returned by int_hook()

returns:
- false if a bad argument is passed
- true if the handler function is succesfully unhooked
*/
bool int_unhook(uint8_t int_n, uint8_t hook_n) {
    if (hook_n >= ISR_MAX_HOOKS) {
        return false;
    }

    isr_hooks[int_n][hook_n] = null;
    return true;
}