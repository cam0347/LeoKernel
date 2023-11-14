#include <io/include/pci.h>
#include <io/include/pcie.h>
#include <io/include/pci_tree.h>
#include <include/types.h>
#include <io/include/port_io.h>
#include <include/mem.h>
#include <acpi/include/acpi_parser.h>
#include <tty/include/tty.h>
#include <mm/include/obj_alloc.h>

bool pcie_supported = false;
pool_t pci_dev_pool_id;
uint32_t pci_dev_pool_last_ind = 0;

bool init_pci() {
    void *mcfg = acpi_locate_table("MCFG", 0);
    pcie_supported = mcfg ? true : false;
    pci_dev_pool_last_ind = 0;

    if (!create_obj_pool(&pci_dev_pool_id, sizeof(pci_device_t), 128, manual)) {
        return false;
    }

    if (pcie_supported) {
        enum_pcie();
    } else {
        enum_pci();
    }

    for (int i = 0; i < pci_dev_pool_last_ind; i++) {
        pci_device_t *dev;
        obj_pool_get(pci_dev_pool_id, (void **) &dev, i);
        pci_dev_header_t *header = (pci_dev_header_t *) dev;

        if ((header->header_type & 0x7F) == general_device) {
            printf("general device %d %d\n", dev->general.header.class_code, dev->general.header.subclass);
        } else if ((header->header_type & 0x7F) == pci_to_pci_bridge) {
            pci_pci_bridge_t *bridge = (pci_pci_bridge_t *) dev;
            printf("pci-to-pci bridge %d %d %d\n", bridge->primary_bus_number, bridge->secondary_bus_number, bridge->subordinate_bus_number);
        } else {
            printf("other %d %d %d [header type, class, subclass]\n", dev->general.header.header_type, dev->general.header.class_code, dev->general.header.subclass);
        }
    }

    return true;
}

void request_bar_size(uint8_t bus, uint8_t dev, uint8_t func) {
    pci_config_read_dword(bus, dev, func, 4);
    pci_config_write_dword(bus, dev, func, 4, 0xFFFFFFFF);
}

//read a dword from the PCI I/O configuration space
uint32_t pci_config_read_dword(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset) {
    outl(PCI_CONFIG_ADDRESS, PCI_ADDRESS(bus, dev, func, offset));
    return inl(PCI_CONFIG_DATA);
}

//writes a dword to the PCI I/O configuration space
void pci_config_write_dword(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint32_t data) {
    outl(PCI_CONFIG_ADDRESS, PCI_ADDRESS(bus, dev, func, offset));
    outl(PCI_CONFIG_DATA, data);
}

void pci_general_device_reverse_endianess(pci_general_dev_t *dev) {
    reverse_endianess(&dev->header.vendor_id, 2);
    reverse_endianess(&dev->header.device_id, 2);
    reverse_endianess(&dev->header.command, 2);
    reverse_endianess(&dev->header.status, 2);
    reverse_endianess(&dev->bar0, 4);
    reverse_endianess(&dev->bar1, 4);
    reverse_endianess(&dev->bar2, 4);
    reverse_endianess(&dev->bar3, 4);
    reverse_endianess(&dev->bar4, 4);
    reverse_endianess(&dev->bar5, 4);
    reverse_endianess(&dev->cardbus_cis_ptr, 4);
    reverse_endianess(&dev->subsystem_vendor_id, 2);
    reverse_endianess(&dev->subsystem_id, 2);
    reverse_endianess(&dev->expansion_rom_base, 4);
}

void pci_ptp_bridge_reverse_endianess(pci_pci_bridge_t *dev) {
    reverse_endianess(&dev->header.vendor_id, 2);
    reverse_endianess(&dev->header.device_id, 2);
    reverse_endianess(&dev->header.command, 2);
    reverse_endianess(&dev->header.status, 2);
    reverse_endianess(&dev->secondary_status, 2);
    reverse_endianess(&dev->memory_base, 2);
    reverse_endianess(&dev->memory_limit, 2);
    reverse_endianess(&dev->prefetchable_memory_base, 2);
    reverse_endianess(&dev->prefetchable_memory_limit, 2);
    reverse_endianess(&dev->prefetchable_base_upper, 4);
    reverse_endianess(&dev->prefetchable_limit_upper, 4);
    reverse_endianess(&dev->io_base_upper, 2);
    reverse_endianess(&dev->io_limit_upper, 2);
    reverse_endianess(&dev->expansion_rom_base, 4);
    reverse_endianess(&dev->bridge_control, 2);
}

//reads a device descriptor from pci i/o configuration space
bool pci_config_read_entry(uint8_t bus, uint8_t dev, uint8_t fnc, pci_device_t *out) {
    for (uint32_t i = 0; i < sizeof(pci_general_dev_t) / 4; i++) {
        *((uint32_t *) out + i) = pci_config_read_dword(bus, dev, fnc, i * 4);
    }

    if ((out->general.header.header_type & 0x7F) == general_device) {
        pci_general_device_reverse_endianess(&out->general);
    } else if ((out->general.header.header_type & 0x7F) == pci_to_pci_bridge) {
        pci_ptp_bridge_reverse_endianess(&out->bridge);
    } else {
        return false;
    }

    if (out->general.header.vendor_id == 0xFFFF) {
        return false;
    }

    return true;
}

//scan the pci lines to find attached devices and put them in the device pool
void enum_pci() {
    for (uint8_t bus = 0; bus < 255; bus++) {
        for (uint8_t dev = 0; dev < 32; dev++) {
            pci_scan_dev(bus, dev);
        }
    }
}

/*
scans all functions of the device on the specified bus and saves
their informations in the pci device pool.
*/
void pci_scan_dev(uint8_t bus, uint8_t dev) {
    for (uint8_t fnc = 0; fnc < 8; fnc++) {
        pci_device_t devfnc;
        
        if (!pci_config_read_entry(bus, dev, fnc, &devfnc)) {
            break; //entry non valida o non presente
        }

        if (devfnc.general.bar0 & 1) {
            printf("I/O bar\n");
        } else {
            printf("memory mapped BAR ");
            uint8_t type = devfnc.general.bar0 >> 1 & 3;

            if (type == 0) {
                printf("32 bit wide ");
            } else if (type == 2) {
                printf("64 bit wide ");
            }

            uint64_t addr = devfnc.general.bar0 >> 4;
            printf("0x%X\n", addr);
        }

        obj_pool_put(pci_dev_pool_id, (void *) &devfnc, pci_dev_pool_last_ind++);
        bool multifunction = devfnc.general.header.header_type >> 7;

        if (!multifunction && fnc == 0) {
            break;
        }
    }
}