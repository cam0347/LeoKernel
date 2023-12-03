#pragma once
#include <include/types.h>
#define PCI_CONFIG_ADDRESS (uint16_t) 0x0CF8
#define PCI_CONFIG_DATA (uint16_t) 0x0CFC
#define PCI_ADDRESS(bus, dev, fnc, off) (((uint32_t) bus & 0xFF) << 16 | ((uint32_t) dev & 0x1F) << 11 | ((uint32_t) fnc & 0x07) << 8 | off & 0xFF | 1 << 31)
#define PCI_FNC_EXIST(bus, dev, fnc) ((pci_config_read(bus, dev, fnc, 0) & 0xFFFF) != 0xFFFF)

typedef struct {
    uint16_t vendor_id;      //identifies the manufacturer of the device
    uint16_t device_id;      //identifies the device
    uint16_t command;        //
    uint16_t status;         //status for pci related events
    uint8_t revision_id;     //revision identifier
    uint8_t prog_if;         //register that specifies a register-level programming interface the device has
    uint8_t subclass;        //identifies the specific function this device performs
    uint8_t class_code;      //identifies the type of function this device performs
    uint8_t cache_line_size;
    uint8_t latency_timer;   //latency of this device in pci bus clock units
    uint8_t header_type;     //identifies the type of the rest of the header (0: general device, 1: pci-to-pci bridge, 2: pci-to-cardbus bridge)
    uint8_t bist;            //status of the built-in self test
} __attribute__((packed)) pci_dev_header_t;

//header type 0x00
typedef struct {
    pci_dev_header_t header;
    uint32_t bar0;
    uint32_t bar1;
    uint32_t bar2;
    uint32_t bar3;
    uint32_t bar4;
    uint32_t bar5;
    uint32_t cardbus_cis_ptr;
    uint16_t subsystem_vendor_id;
    uint16_t subsystem_id;
    uint32_t expansion_rom_base;
    uint8_t capabilities_ptr;
    uint8_t res0[3];
    uint32_t res1;
    uint8_t int_line;
    uint8_t int_pin;
    uint8_t min_grant;
    uint8_t max_latency;
} __attribute__((packed)) pci_general_dev_t;

//header type 0x01
typedef struct {
    pci_dev_header_t header;
    uint32_t bar0;
    uint32_t bar1;
    uint8_t primary_bus_number;            //upstream bus number
    uint8_t secondary_bus_number;          //downstream bus number
    uint8_t subordinate_bus_number;        //the highest bus number of all of the buses that can be reached downstream of the bridge
    uint8_t secondary_latency_timer;
    uint8_t io_base;
    uint8_t io_limit;
    uint16_t secondary_status;
    uint16_t memory_base;
    uint16_t memory_limit;
    uint16_t prefetchable_memory_base;
    uint16_t prefetchable_memory_limit;
    uint32_t prefetchable_base_upper;
    uint32_t prefetchable_limit_upper;
    uint16_t io_base_upper;
    uint16_t io_limit_upper;
    uint8_t capability_ptr;
    uint8_t res0[3];
    uint32_t expansion_rom_base;
    uint8_t int_line;
    uint8_t int_pin;
    uint16_t bridge_control;
} __attribute__((packed)) pci_pci_bridge_t;

typedef enum {
    general_device = 0,
    pci_to_pci_bridge = 1,
    pci_to_cardbus_bridge = 2
} pci_header_type_t;

typedef enum {
    unclassified = 0x00,
    mass_storage = 0x01,
    network_controller = 0x02,
    display = 0x03,
    multimedia_controller = 0x04,
    memory_controller = 0x05,
    bridge = 0x06,
    simple_comm_controller = 0x07,
    base_sys_peripheral = 0x08,
    input_device = 0x09,
    docking_station = 0x0A,
    processor = 0x0B,
    serial_bus = 0x0C,
    wireless_controller = 0x0D,
    intelligent_controller = 0x0E,
    satellite_comm_controller = 0x0F,
    encryption_controller = 0x10,
    signal_processing_controller = 0x11,
    processing_accelerator = 0x12,
    non_essential_instrumentation = 0x13,
    coprocessor = 0x40,
    unassigned = 0xFF
} pci_dev_type_t;

typedef union {
    pci_general_dev_t general;
    pci_pci_bridge_t bridge;
} pci_device_t;

//header type 0x02 not implemented

bool init_pci();
uint32_t pci_config_read_dword(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset);
void pci_config_write_dword(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint32_t data);
void pci_general_device_reverse_endianess(pci_general_dev_t *dev);
void pci_ptp_bridge_reverse_endianess(pci_pci_bridge_t *dev);
bool pci_config_read_entry(uint8_t bus, uint8_t dev, uint8_t fnc, pci_device_t *out);
void enum_pci();
void pci_scan_dev(uint8_t bus, uint8_t dev);
uint32_t pci_read_capability_register(pci_general_dev_t *dev, uint8_t reg);
bool pci_init_device(pci_general_dev_t *dev);