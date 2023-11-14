#pragma once
#include <include/types.h>

#define DEFAULT_LAPIC_ADDRESS (void *) 0xFEE00000
#define APIC_ARRAYS_LENGTH 16

/* lapic registers */

#define LAPIC_EOI_REGISTER 0xB0 //end of interrupt register
#define LAPIC_SIV_REGISTER 0xF0 //spurious interrupt vector register
#define APIC_TIMER_ICR 0x380
#define APIC_TIMER_CCR 0x390
#define APIC_TIMER_DCR 0x3E0
#define LAPIC_LVT_MACHINE_CHECK 0x2F0
#define LAPIC_LVT_TIMER 0x320
#define LAPIC_LVT_THERMAL 0x330
#define LAPIC_LVT_PERF_MON 0x340
#define LAPIC_LVT_LINT0 0x350
#define LAPIC_LVT_LINT1 0x360
#define LAPIC_LVT_ERROR 0x370

/* lapic timer divider values */

#define LAPIC_TIMER_DIV1 7
#define LAPIC_TIMER_DIV2 0
#define LAPIC_TIMER_DIV4 1
#define LAPIC_TIMER_DIV8 2
#define LAPIC_TIMER_DIV16 3
#define LAPIC_TIMER_DIV32 4
#define LAPIC_TIMER_DIV64 5
#define LAPIC_TIMER_DIV128 6

/* apic timer modes */

#define LAPIC_TIMER_ONE_SHOT_MODE 0
#define LAPIC_TIMER_PERIODIC_MODE 1
#define LAPIC_TIMER_TSC_MODE 2

typedef struct {
    uint8_t cpu_id;
    uint8_t lapic_id;
    uint32_t flags; //bit 0: processor enabled, bit 1: online capable
} lapic_t;

typedef struct {
    uint8_t ioapic_id;
    uint32_t ioapic_addr;
    uint32_t global_system_interrupt_base;
    uint8_t max_redirections;
} ioapic_t;

typedef enum {
    fixed = 0,
    low_priority = 1,
    smi = 2,
    nmi = 4,
    init = 5,
    extint = 7
} ioapic_int_delivery_mode_t;

typedef enum {
    physical = 0,
    logical = 1
} ioapic_int_destination_mode_t;

typedef enum {
    delivered = 0,
    not_delivered = 1
} ioapic_int_delivery_status_t;

typedef enum {
    active_high = 0,
    active_low = 1
} ioapic_int_pin_polarity_t;

typedef enum {
    edge = 0,
    level = 1
} ioapic_int_trigger_mode_t;

/* generic functions */

bool init_apic();
void clear_int_redirection_table();
bool check_apic_addresses();
void disable_pic();
bool check_apic();

/* local apic functions */

uint32_t lapic_read(uint32_t reg);
void lapic_write(uint32_t reg, uint32_t data);
void lapic_set_lvt_entry(uint32_t reg, uint8_t int_num, uint8_t polarity, uint8_t trigger, bool masked);
void save_lapic_info(uint8_t lapic_id, uint8_t cpu_id, uint32_t flags);

/* io apic functions */

ioapic_t *get_ioapic_by_irq(uint8_t irq);
uint32_t ioapic_read(uint8_t id, uint32_t reg);
void ioapic_write(uint8_t id, uint32_t reg, uint32_t data);
bool ioapic_read_irq_entry(uint8_t irq, uint64_t *entry);
void save_ioapic_info(uint8_t ioapic_id, uint32_t addr, uint32_t gsib);

/* apic timer functions */

void apic_timer_init();
void apic_timer_mode(uint8_t mode);
void apic_timer_div(uint8_t div);
void apic_timer_set_count(uint32_t count);
uint32_t apic_timer_get_count();
void apic_sleep(uint32_t ms);
void apic_timer_hook();

/* other */

void send_eoi();
bool apic_irq(uint8_t irq, uint8_t int_vector, ioapic_int_delivery_mode_t del_mode, uint8_t dest_mode, uint8_t polarity, uint8_t trigger_mode, bool masked, uint8_t destination);
bool apic_clear_irq(uint8_t irq);
bool apic_set_mask(uint8_t irq, bool masked);
void apic_lvt_set_mask(uint32_t reg, bool masked);