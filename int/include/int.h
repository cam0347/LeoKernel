#pragma once
#include <include/types.h>
#define ISR_TYPE_INT 0x0E
#define ISR_TYPE_TRAP 0x0F
#define ISR_FLAG_PRESENT 0x80
#define ISR_FLAG_RING0 0x00
#define ISR_FLAG_RING1 0x20
#define ISR_FLAG_RING2 0x40
#define ISR_FLAG_RING3 0x60
#define MAX_INTERRUPT 256
#define NMI_CONTROL_REGISTER 0x70
#define ISR_MAX_HOOKS 4

typedef struct {
	uint16_t isr_0;
	uint16_t kernel_cs;
	uint8_t ist; //offset into interrupt stack table (ist), if set to 0 the ist is not used
	uint8_t flags; //0-3 gate type, 4 unset, 5-6 dpl, 7 present
	uint16_t isr_1;
	uint32_t isr_2;
	uint32_t reserved;
} __attribute__((packed)) idt_t;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idtr_t;

typedef struct {
	void *hooks[ISR_MAX_HOOKS];
} isr_hook_t;

bool setup_interrupts();
void set_idt_entry(uint8_t n, uint64_t isr, uint8_t flags);
void nmi_enable();
void nmi_disable();
uint8_t int_hook(uint8_t int_n, void *handler);
bool int_unhook(uint8_t int_n, uint8_t hook_n);