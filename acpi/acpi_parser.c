#include <include/types.h>
#include <acpi/include/acpi.h>
#include <acpi/include/acpi_parser.h>
#include <include/bootp.h>
#include <include/string.h>
#include <include/mem.h>
#include <tty/include/tty.h>
#include <int/include/apic.h>

acpi_xsdt_t *__acpi_xsdt;
acpi_fadt_t *__acpi_fadt;
acpi_madt_t *__acpi_madt;
void *__acpi_dsdt;
void *__acpi_ssdt;
void *__acpi_facs;
acpi_preferred_power_management_profile __machine_type;
bool __acpi_manager_ready = false;

bool init_acpi(struct leokernel_boot_params bootp) {
    __acpi_xsdt = (acpi_xsdt_t *) bootp.xsdt;
    __acpi_manager_ready = true;
    __acpi_fadt = (acpi_fadt_t *) acpi_locate_table("FACP", 0);
    __acpi_madt = (acpi_madt_t *) acpi_locate_table("APIC", 0);
    if (__acpi_fadt == null || __acpi_madt == null) {return false;}
    parse_fadt(__acpi_fadt); //sets __acpi_dsdt and __acpi_facs
    if (__acpi_dsdt == null) {return false;}

    extern uint32_t lapics_array_index; //defined in apic.c
    lapics_array_index = 0;
    parse_madt(__acpi_madt);
    return true;
}

/*
Seeks the n-th ACPI table that matches the given signature.
There could be more tables with the same signature, that's the need of having an index.
*/
void *acpi_locate_table(char *signature, uint32_t n) {
    if (!__acpi_manager_ready) {
        return null;
    }

    if (strncmp(signature, "DSDT", 4) == 0) {
        return __acpi_dsdt;
    }

    uint32_t num_entries = (__acpi_xsdt->h.length - sizeof(acpi_table_header)) / sizeof(void *);
    uint32_t last_index = 0;

    for (uint32_t i = 0; i < num_entries; i++) {
        acpi_table_header *table = __acpi_xsdt->ptr[i];
        if (!validate_checksum(table)) {continue;}
        if (strncmp(table->signature, signature, 4) == 0) {
            if (last_index == n) {
                return (void *) table;
            } else {
                last_index++;
            }
        }
    }

    return null;
}

bool validate_checksum(acpi_table_header *header) {
    uint8_t sum = 0;
 
    for (int i = 0; i < header->length; i++) {
        sum += ((char *) header)[i];
    }
 
    return sum == 0;
}

void parse_fadt(void *fadt_addr) {
    acpi_fadt_t *fadt = (acpi_fadt_t *) fadt_addr;
    __acpi_dsdt = (void *) fadt->x_dsdt;
    __acpi_facs = (void *)(uint64_t) fadt->firmware_ctrl != 0 ? (void *)(uint64_t) fadt->firmware_ctrl : (void *) fadt->x_firmware_ctrl;
    __machine_type = (acpi_preferred_power_management_profile) fadt->preferred_pm_profile;
}

void parse_madt(void *madt_addr) {
    acpi_madt_t *madt = (acpi_madt_t *) madt_addr;
    void *madt_entries = (void *) madt + sizeof(acpi_madt_t); //the entries in this table start after the header
    uint64_t offset = 0;

    extern void *lapic_address; //defined in apic.c
    lapic_address = (void *)(uint64_t) madt->local_apic_addr;

    while(offset < madt->h.length - sizeof(acpi_madt_t)) {
        void *entry = madt_entries + offset;
        uint8_t entry_type = *(uint8_t *) entry;
        uint8_t entry_size = *(uint8_t *) (entry + 1);
        switch_madt_entry(entry, entry_type);
        offset += entry_size;
    }
}

/*
Do something with a madt entry
*/
void switch_madt_entry(void *entry, acpi_madt_entry_type type) {
    switch((acpi_madt_entry_type) type) {
        //defines a local apic
        case lapic:
            acpi_madt_entry_lapic *lapic = (acpi_madt_entry_lapic *) entry;
            save_lapic_info(lapic->apic_id, lapic->acpi_cpu_id, lapic->flags); //save this lapic info in the array defined in apic.c
            break;

        //defines an i/o apic
        case io_apic:
            acpi_madt_entry_io_apic *ioapic = (acpi_madt_entry_io_apic *) entry;
            save_ioapic_info(ioapic->io_apic_id, ioapic->io_apic_addr, ioapic->global_system_interrupt_base); //save this ioapic info in the array defined in apic.c
            break;

        //defines an i/o interrupt map to the gsi
        case ioapic_int_map:
            acpi_madt_entry_io_source_override *io_source_override = (acpi_madt_entry_io_source_override *) entry;
            extern uint8_t gsi_map[]; //defined in int.c
            gsi_map[io_source_override->irq_source] = io_source_override->global_system_interrupt;
            break;

        //defines an i/o apic source that should me marked as non-maskable
        case ioapic_nmi:
            acpi_madt_entry_nmi_source *nmi_source = (acpi_madt_entry_nmi_source *) entry;
            printf("ioapic non-maskable -> irq: %d, int: %d [UNHANDLED]\n", nmi_source->nmi_source, nmi_source->global_system_interrupt);
            //......
            break;

        //specifies for each processor which lapic input is connected to nmi (how should i use this information?)
        case lapic_nmi:
            acpi_madt_entry_lapic_nmi *lapic_nmi = (acpi_madt_entry_lapic_nmi *) entry;
            //printf("lapic non-maskable -> cpu id: %d, LINT#: %d\n", lapic_nmi->acpi_cpu_id, lapic_nmi->lint01);
            //.......
            break;

        //defines an override address of the local apic
        case lapic_addr_override:
            acpi_madt_entry_lapic_addr_override *lapic_addr_ovr = (acpi_madt_entry_lapic_addr_override *) entry;
            extern void *lapic_address; //defined in apic.c
            lapic_address = (void *) lapic_addr_ovr->lapic_phys_addr;
            break;

        default:
            printf("Unknown MADT entry: %d\n", (uint32_t) type);
            //we should do something...
            break;
    }
}