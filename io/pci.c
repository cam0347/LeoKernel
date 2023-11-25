#include <io/include/pci.h>
#include <io/include/pcie.h>
#include <io/include/pci_tree.h>
#include <include/types.h>
#include <io/include/port_io.h>
#include <include/mem.h>
#include <acpi/include/acpi_parser.h>
#include <tty/include/tty.h>
#include <mm/include/obj_alloc.h>
#include <include/assert.h>
#include <io/include/devs.h>
#include <drv/ide/include/ide.h>

bool pcie_supported = false;
extern pool_t ide_controllers_pool_id; //defined in ide.c

bool init_pci() {
    void *mcfg = acpi_locate_table("MCFG", 0);
    pcie_supported = mcfg ? true : false;

    if (!init_pci_tree()) {
        return false;
    }

    //create some pools
    if (!create_obj_pool(&ide_controllers_pool_id, sizeof(ide_controller_t), 8, manual)) {return false;}

    assert_true(pcie_supported); //---------------------------------- to be removed ------------------------------------------

    if (pcie_supported) {
        if (!enum_pcie()) {
            return false;
        }
    } else {
        enum_pci();
    }

    return true;
}

/* configures the device, called by pci/pcie bus scan functions */
bool pci_init_device(pci_general_dev_t *dev) {
    if (!dev) {
        return false;
    }

    switch((pci_dev_type_t) dev->header.class_code) {
        case mass_storage:
            return devs_config_mass_storage(dev);
            break;
    }
}

uint32_t pci_read_capability_register(pci_general_dev_t *dev, uint8_t reg) {
    if (!dev->capabilities_ptr) {
        return 0;
    }

    /*
    the capabilities pointer is an offset into this device's configuration space,
    the lowest 2 bits of that field are reserved and must me masked.
    The register (reg) selects a 4-byte register into the capabilities list entry.
    */
    uint32_t *data = (void *) dev + (dev->capabilities_ptr & 0xFC) + reg * 4;
    return *data;
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

        bool multifunction = devfnc.general.header.header_type >> 7;

        if (!multifunction && fnc == 0) {
            break;
        }
    }
}