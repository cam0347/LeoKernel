#pragma once
#include <include/types.h>

typedef struct {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char OEMID[6];
    char OEMTableID[8];
    uint32_t OEMRevision;
    uint32_t creatorID;
    uint32_t creatorRevision;
} __attribute__((packed)) acpi_table_header;

//extended 64 bit rsdt (xsdt)
typedef struct  {
    acpi_table_header h;
    void *ptr[];
    //64 bit pointers to other tables
} __attribute__((packed)) acpi_xsdt_t;

/*
Generic Address Structure (GAS).
This is used to express register addresses within tables defined by ACPI.
*/
typedef struct {
    uint8_t address_space_id;
    uint8_t register_bit_width;
    uint8_t register_bit_offset;
    uint8_t access_size;
    uint64_t address;
} __attribute__((packed)) acpi_gas_t;

typedef enum {
    system_memory,
    system_io,
    pci_config_space,
    embedded_controller,
    system_management_bus,
    system_cmos,
    pci_bar_target,
    ipmi, //intelligent platform management infrastructure
    gpio,
    generic_serial_bus,
    pcc //platform communication channel
} acpi_gas_address_space_type;

typedef enum {
    undefined,
    bits_8,
    bits_16,
    bits_32,
    bits_64
} acpi_gas_access_size_type;

typedef struct {
    acpi_table_header h;
    uint32_t firmware_ctrl;
    uint32_t dsdt;
    uint8_t reserved_0;
    uint8_t preferred_pm_profile;
    uint16_t sci_int;
    uint32_t smi_cmd;
    uint8_t acpi_enable;
    uint8_t acpi_disable;
    uint8_t s4bios_req;
    uint8_t pstate_cnt;
    uint32_t pm1a_evt_blk;
    uint32_t pm1b_evt_blk;
    uint32_t pm1a_cnt_blk;
    uint32_t pm1b_cnt_blk;
    uint32_t pm2_cnt_blk;
    uint32_t pm_tmr_blk;
    uint32_t gpe0_blk;
    uint32_t gpe1_blk;
    uint8_t pm1_evt_len;
    uint8_t pm1_cnt_len;
    uint8_t pm2_cnt_len;
    uint8_t pm_tmr_len;
    uint8_t gpe0_blk_len;
    uint8_t gpe1_blk_len;
    uint8_t gpe1_base;
    uint8_t cst_cnt;
    uint16_t p_lvl2_lat;
    uint16_t p_lvl3_lat;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t duty_offset;
    uint8_t duty_width;
    uint8_t day_alrm;
    uint8_t mon_alrm;
    uint8_t century;
    uint16_t iapc_boot_arch;
    uint8_t reserved_1;
    uint32_t flags;
    acpi_gas_t reset_reg;
    uint8_t reset_value;
    uint16_t arm_boot_arch;
    uint8_t minor_version;
    uint64_t x_firmware_ctrl;
    uint64_t x_dsdt;
    acpi_gas_t x_pm1a_evt_blk;
    acpi_gas_t x_pm1b_evt_blk;
    acpi_gas_t x_pm1a_cnt_blk;
    acpi_gas_t x_pm1b_cnt_blk;
    acpi_gas_t x_pm2_cnt_blk;
    acpi_gas_t x_pm_tmr_blk;
    acpi_gas_t x_gpe0_blk;
    acpi_gas_t x_gpe1_blk;
    acpi_gas_t sleep_control_reg;
    acpi_gas_t sleep_status_register;
    uint64_t hypervisor_vendor_identity;
} __attribute__((packed)) acpi_fadt_t;

typedef struct {
    acpi_table_header h;
    //length - sizeof(acpi_dsdt_t) bytes of aml code...
} __attribute__((packed)) acpi_dsdt_t;

typedef enum {
    unspecified,
    desktop,
    mobile,
    workstation,
    enterprise_server,
    soho_server,
    appliance_pc,
    performance_server
} acpi_preferred_power_management_profile;

typedef struct {
    acpi_table_header h;
    uint32_t local_apic_addr;
    uint32_t flags;
    /*
    after this, there's a list of variable length descriptors.
    every descriptor starts with two bytes: the first describes the type, the seconds describes the length of that descriptor
    */
} __attribute__((packed)) acpi_madt_t;

typedef struct {
    struct {uint8_t type; uint8_t size;} header;
    uint8_t acpi_cpu_id;
    uint8_t apic_id;
    uint32_t flags;
} __attribute__((packed)) acpi_madt_entry_lapic;

typedef struct {
    struct {uint8_t type; uint8_t size;} header;
    uint8_t io_apic_id;
    uint8_t reserved;
    uint32_t io_apic_addr;
    uint32_t global_system_interrupt_base;
} __attribute__((packed)) acpi_madt_entry_io_apic;

typedef struct {
    struct {uint8_t type; uint8_t size;} header;
    uint8_t bus_source;
    uint8_t irq_source;
    uint32_t global_system_interrupt;
    uint16_t flags;
} __attribute__((packed)) acpi_madt_entry_io_source_override;

//this may be wrong
typedef struct {
    struct {uint8_t type; uint8_t size;} header;
    uint8_t nmi_source;
    uint8_t reserved;
    uint16_t flags;
    uint32_t global_system_interrupt;
} __attribute__((packed)) acpi_madt_entry_nmi_source;

typedef struct {
    struct {uint8_t type; uint8_t size;} header;
    uint8_t acpi_cpu_id; //if 0xFF means all processors
    uint16_t flags;
    uint8_t lint01;
} __attribute__((packed)) acpi_madt_entry_lapic_nmi;

typedef struct {
    struct {uint8_t type; uint8_t size;} header;
    uint16_t reserved;
    uint64_t lapic_phys_addr;
} __attribute__((packed)) acpi_madt_entry_lapic_addr_override;

typedef struct {
    struct {uint8_t type; uint8_t size;} header;
    uint16_t reserved;
    uint32_t local_x2apic_id;
    uint32_t flags;
    uint32_t acpi_id;
} __attribute__((packed)) acpi_madt_entry_local_x2apic;

typedef enum {
    lapic,
    io_apic,
    ioapic_int_map,
    ioapic_nmi,
    lapic_nmi,
    lapic_addr_override,
    local_x2apic = 9
} acpi_madt_entry_type;

typedef struct {
    uint64_t base;
    uint16_t segment_group_number;
    uint8_t start_bus;
    uint8_t end_bus;
    uint32_t reserved;
} __attribute__((packed)) acpi_mcfg_entry_t;

typedef struct {
    acpi_table_header h;
    uint64_t reserved;
    acpi_mcfg_entry_t entries[];
} __attribute__((packed)) acpi_mcfg_t;