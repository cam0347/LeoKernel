/*
In an APIC-based system, each CPU is made of a "core" and a "local APIC".
The local APIC is responsible for handling cpu-specific interrupt configuration.
Among other things, it contains the Local Vector Table (LVT) that translates events such as "internal clock" and other "local" interrupt sources
into a interrupt vector (e.g. LocalINT1 pin could be raising an NMI exception by storing "2" in the corresponding entry of the LVT).
In addition, there is an I/O APIC (e.g. intel 82093AA) that is part of the chipset and provides multi-processor interrupt management, incorporating both
static and dynamic symmetric interrupt distribution across all processors. In systems with multiple I/O subsystems, each subsystem can have
its own set of interrupts.
Each interrupt pin is individually programmable as either edge or level triggered.
The interrupt vector and interrupt steering information can be specified per interrupt.
An indirect register accessing scheme optimizes the memory space needed to access the I/O APIC's internal registers.
To increase system flexibility when assigning memory space usage, the I/O APIC's two-register memory space is relocatable, but defaults to 0xFEC00000.
*/

#include <include/types.h>
#include <int/include/apic.h>
#include <int/include/pic.h>
#include <mm/include/obj_alloc.h>
#include <io/include/port_io.h>
#include <include/low_level.h>
#include <include/panic.h>
#include <tty/include/tty.h>
#include <mm/include/paging.h>
#include <include/sleep.h>
#include <int/include/int.h>

void *lapic_address = null; //lapic registers physical frame
lapic_t system_lapics[APIC_ARRAYS_LENGTH]; //contains lapic descriptors
ioapic_t system_ioapics[256]; //contains ioapic descriptors
uint32_t lapics_array_index;

/* apic timer variables */
bool apic_sleep_ready = false;
uint32_t apic_timer_frequency = 0;
bool apic_sleep_in_progress = false;

/*
Initialize PIC/APIC system.
When this function is called, lapic_address should already be set when the madt table has been parsed.
If not, that variable is initialized to its default value.
*/
bool init_apic() {
    if (!check_apic()) {
        return false;
    }

    disable_pic(); //to use apic we have to disable pic first

    if (lapic_address == null) {
        lapic_address = DEFAULT_LAPIC_ADDRESS;
    }

    if (get_physical_address(lapic_address) == TRANSLATION_UNKNOWN) {
        printf("APIC addresses not mapped to virtual memory (0x%X)\n", lapic_address);
        return false;
    }

    //sets the spurious interrupt vector register
    lapic_write(LAPIC_SIV_REGISTER, 0x1FF);

    //set some local vector table entries
    lapic_set_lvt_entry(LAPIC_LVT_TIMER, 0x19, 1, 0, true); //apic timer
    lapic_set_lvt_entry(LAPIC_LVT_LINT0, 0x2, 0, 0, false); //lint0 nmi
    lapic_set_lvt_entry(LAPIC_LVT_LINT1, 0x2, 0, 0, false); //lint1 nmi

    apic_sleep_ready = false;
    apic_timer_frequency = 0;
    apic_sleep_in_progress = false;

    return true;
}

//returns the descriptor of the ioapic that can handle the specified irq number if found
ioapic_t *get_ioapic_by_irq(uint8_t irq) {
    for (uint32_t i = 0; i < 256; i++) {
        ioapic_t *entry = &system_ioapics[i];

        //if this ioapic doesn't exist, continue
        if (entry->ioapic_addr == null) {
            continue;
        }

        uint32_t gsi_base = entry->global_system_interrupt_base;

        //r u sure about these constraints?
        if (gsi_base <= irq && irq - gsi_base < entry->max_redirections) {
            return entry;
        }
    }

    return null;
}

uint32_t ioapic_read(uint8_t id, uint32_t reg) {
    ioapic_t *ioapic_struct = &system_ioapics[id];

    //checks if this entry exists
    if (ioapic_struct->ioapic_id != id) {
        panic("illegal I/O apic id referenced");
        return 0;
    }

    /*volatile uint32_t *addr_register = (volatile uint32_t *) ioapic_address;
    volatile uint32_t *data_register = (volatile uint32_t *) (ioapic_address + 0x10);*/
    volatile uint32_t *addr_register = (volatile uint32_t *)(uint64_t) ioapic_struct->ioapic_addr;
    volatile uint32_t *data_register = (volatile uint32_t *)(uint64_t)(ioapic_struct->ioapic_addr + 0x10);
    *addr_register = reg & 0xFF; //the lower 8 bits of address register hold the register number
    return *data_register;
}
 
void ioapic_write(uint8_t id, uint32_t reg, uint32_t data) {
    ioapic_t *ioapic_struct = &system_ioapics[id];

    //checks if this entry exists
    if (ioapic_struct->ioapic_id != id) {
        panic("illegal I/O apic id referenced");
        return;
    }

    volatile uint32_t *addr_register = (volatile uint32_t *)(uint64_t) ioapic_struct->ioapic_addr;
    volatile uint32_t *data_register = (volatile uint32_t *)(uint64_t)(ioapic_struct->ioapic_addr + 0x10);

    /*uint32_t volatile *addr_register = (uint32_t volatile *) ioapic_address;
    uint32_t volatile *data_register = (uint32_t volatile *) (ioapic_address + 0x10);*/
    *addr_register = reg & 0xFF; //the lower 8 bits of address register hold the register number
    *data_register = data;
}

/*
sets an irq routing rule.
parameters:
- irq: source IRQ number
- int_vector: destination interrupt vector
- del_mode: delivery mode (fixed/low_priority/smi/nmi/init/extint)
- dest_mode: destination mode (physical/logical (unsupported))
- polarity: irq pin polarity (active_high/active_low)
- trigger_mode: (edge/level)
- masked: true if this interrupt will be masked
- destination: if destination mode is set to physical, the lowest 4 bits contain the APIC id of the processor to send the interrupt to, if destination mode
                is set to logical this field contains a group of processors (unsupported)
*/
bool apic_irq(uint8_t irq, uint8_t int_vector, ioapic_int_delivery_mode_t del_mode, uint8_t dest_mode, uint8_t polarity, uint8_t trigger_mode, bool masked, uint8_t destination) {
    ioapic_t *ioapic_struct;

    //if a suitable ioapic is not found, return false
    if (!(ioapic_struct = get_ioapic_by_irq(irq))) {
        return false;
    }

    uint32_t entry_lo = ioapic_read(ioapic_struct->ioapic_id, 0x10 + 2 * irq) & 0xFFFE5000;
    uint32_t entry_hi = ioapic_read(ioapic_struct->ioapic_id, 0x10 + 2 * irq + 1) & 0xF0FFFFFF;

    entry_lo |= (uint32_t) int_vector & 0xFF;           //interrupt vector bits 0:7
    entry_lo |= ((uint32_t) del_mode & 7) << 8;         //delivery mode bits 8:10
    entry_lo |= ((uint32_t) dest_mode & 1) << 11;       //destination mode bit 11
    entry_lo |= ((uint32_t) polarity & 1) << 13;        //polarity bit 13
    entry_lo |= ((uint32_t) trigger_mode & 1) << 15;    //trigger mode bit 15
    entry_lo |= ((uint32_t) masked & 1) << 16;          //masked bit 16
    entry_hi |= ((uint32_t) destination & 0x0F) << 24;  //destination field bits 56:59

    ioapic_write(ioapic_struct->ioapic_id, 0x10 + 2 * irq, entry_lo);
    ioapic_write(ioapic_struct->ioapic_id, 0x10 + 2 * irq + 1, entry_hi);

    return true;
}

/* masks or unmasks an irq */
bool apic_set_mask(uint8_t irq, bool masked) {
    ioapic_t *ioapic_struct;

    //if a suitable ioapic is not found, return false
    if (!(ioapic_struct = get_ioapic_by_irq(irq))) {
        return false;
    }

    uint32_t entry_lo = ioapic_read(ioapic_struct->ioapic_id, 0x10 + 2 * irq) & 0xFFFEFFFF;
    entry_lo |= ((uint32_t) masked & 1) << 16;
    ioapic_write(ioapic_struct->ioapic_id, 0x10 + 2 * irq, entry_lo);
    return true;
}

//reads a register mapped at lapic_address, the registers are 4 bytes long but 16 byte aligned
uint32_t lapic_read(uint32_t reg) {
    return *(uint32_t *)(lapic_address + reg);
}

void lapic_write(uint32_t reg, uint32_t data) {
    *(uint32_t *)(lapic_address + reg) = data;
}

/*
set a local vector table entry.
parameters:
- reg: register number of the entry
- int_num: interrupt vector
- polarity: 0 active low, 1 active high
- trigger: 0 edge triggered, 1 level triggered
- masked: true if this interrupt should be masked
*/
void lapic_set_lvt_entry(uint32_t reg, uint8_t int_num, uint8_t polarity, uint8_t trigger, bool masked) {
    if (reg < 0x2F0 || reg > 0x370) {
        return;
    }

    uint32_t data = lapic_read(reg) & 0xFFFE5800;
    data |= int_num;
    data |= ((uint32_t) masked & 1) << 16;

    if (reg == LAPIC_LVT_TIMER) {
        if (!masked) {
            data |= 1 << 10;
        }

        data |= ((uint32_t) !polarity & 1) << 13;
        data |= ((uint32_t) trigger & 1) << 15;
    }

    lapic_write(reg, data);
}

void disable_pic() {
    outb(SLAVE_PIC_DATA, 0xFF);
    outb(MASTER_PIC_DATA, 0xFF);
}

bool check_apic() {
    uint32_t eax, edx;
    cpuid(1, &eax, null, null, &edx);
    return edx >> 9 & 1;
}

/* called by an isr to acknowledge an irq */
inline void send_eoi() {
    lapic_write(LAPIC_EOI_REGISTER, 0);
}

void save_lapic_info(uint8_t lapic_id, uint8_t cpu_id, uint32_t flags) {
    if (lapics_array_index >= APIC_ARRAYS_LENGTH) {
        return;
    }

    lapic_t *entry = &system_lapics[lapics_array_index++];
    entry->cpu_id = cpu_id;
    entry->lapic_id = lapic_id;
    entry->flags = flags;
}

void save_ioapic_info(uint8_t ioapic_id, uint32_t addr, uint32_t gsib) {
    ioapic_t *entry = &system_ioapics[ioapic_id];
    entry->ioapic_id = ioapic_id;
    entry->ioapic_addr = addr;
    entry->global_system_interrupt_base = gsib;
    entry->max_redirections = ioapic_read(ioapic_id, 1) >> 16 & 0xFF; //at this point ioapic_read() can be used with this same id
}

/*
initialize apic-supported sleep by approximating the frequency of the apic timer.
the apic-supported sleep won't work until this function is called.
*/
void apic_timer_init() {
    //approximate the apic timer ticks/ms
    apic_timer_div(LAPIC_TIMER_DIV1);
    apic_timer_mode(LAPIC_TIMER_PERIODIC_MODE);
    apic_timer_set_mask(false);

    uint64_t avg = 0;
    for (uint32_t i = 0; i < 100; i++) {
        apic_timer_set_count(0xFFFFFFFF);
        sleep(1);
        uint32_t count = 0xFFFFFFFF - apic_timer_get_count();
        avg += count;
    }

    apic_timer_set_mask(true);
    avg /= 100;
    apic_timer_frequency = avg;
    apic_sleep_ready = true; //from now on the system can use the apic timer to sleep a certain amount of milliseconds
    printf("apic timer frequency: %d\n", apic_timer_frequency);
}

/*
set APIC timer mode (2 bits)
0: one-shot
1: periodic
2: TSC-deadline
3: reserved
*/
void apic_timer_mode(uint8_t mode) {
    uint32_t lvt_entry = lapic_read(LAPIC_LVT_TIMER) & 0xFFF9FFFF; //reset bits 17-18
    lvt_entry |= (mode & 3) << 17;
    lapic_write(LAPIC_LVT_TIMER, lvt_entry);
}

/*
sets divide configuration register (powers of 2 between 1 and 128) */
void apic_timer_div(uint8_t div) {
    uint32_t reg = lapic_read(APIC_TIMER_DCR) & 0xFFFFFFF4;
    reg |= (div & 3) | ((div & 4) << 1);
    lapic_write(APIC_TIMER_DCR, reg);
}

/* set apic timer initial count register */
void apic_timer_set_count(uint32_t count) {
    lapic_write(APIC_TIMER_ICR, count);
}

/* returns apic timer current count */
uint32_t apic_timer_get_count() {
    return lapic_read(APIC_TIMER_CCR);
}

/* masks or unmasks the lapic timer interrupt */
void apic_timer_set_mask(bool masked) {
    uint32_t reg = lapic_read(LAPIC_LVT_TIMER) & 0xFFFEFFFF; //reset bit 16
    reg |= ((uint32_t) masked) << 16;
    lapic_write(LAPIC_LVT_TIMER, reg);
}

void apic_sleep(uint32_t ms) {
    if (!apic_sleep_ready) {
        return;
    }

    apic_timer_div(LAPIC_TIMER_DIV1);
    apic_timer_mode(LAPIC_TIMER_PERIODIC_MODE);
    apic_timer_set_mask(false);
    uint8_t hook_n = int_hook(0x19, (void *) apic_timer_hook);

    while(ms > 0) {
        apic_sleep_in_progress = true;
        apic_timer_set_count(apic_timer_frequency);
        while(apic_sleep_in_progress);
        ms--;
    }

    int_unhook(0x19, hook_n);
    apic_timer_set_mask(true);
}

/* used to implement apic sleep */
void apic_timer_hook() {
    apic_sleep_in_progress = false;
}