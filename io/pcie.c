#include <io/include/pcie.h>
#include <include/types.h>
#include <mm/include/obj_alloc.h>
#include <io/include/pci_tree.h>
#include <acpi/include/acpi.h>
#include <acpi/include/acpi_parser.h>
#include <include/assert.h>

extern pool_t pci_dev_pool_id;          //defined in pci.c
extern uint32_t pci_dev_pool_last_ind;  //defined in pci.c
extern pci_tree_bus_t *pci_tree_root;   //defined in pci_tree.c

/* enumerate devices using pcie configuration mechanism */
bool enum_pcie() {
    acpi_mcfg_t *mcfg;
    
    if (!(mcfg = (acpi_mcfg_t *) acpi_locate_table("MCFG", 0))) {
        return false;
    }

    return pcie_iterate_mcfg(mcfg);
}

bool pcie_iterate_mcfg(acpi_mcfg_t *mcfg) {
    if (!mcfg) {
        return false;
    }

    uint32_t n_entries = (mcfg->h.length - sizeof(acpi_table_header) - 8) / sizeof(acpi_mcfg_entry_t);

    for (uint32_t i = 0; i < n_entries; i++) {
        acpi_mcfg_entry_t *entry = &mcfg->entries[i];

        if (!pcie_scan_bus((void *) entry->base, entry->start_bus)) {
            return false;
        }
    }
}

bool pcie_scan_bus(void *config_base, uint8_t bus) {
    for (uint8_t dev = 0; dev < 32; dev++) {
        if (!pcie_scan_device(config_base, bus, dev)) {
            return false;
        }
    }

    return true;
}

/* scans a device functions and adds them to the devices pool */
bool pcie_scan_device(void *config_base, uint8_t bus, uint8_t dev) {
    for (uint8_t fnc = 0; fnc < 8; fnc++) {
        pci_device_t *device = PCIE_FNC_ADDRESS(config_base, bus, dev, fnc);               //gets the address to the data structure in the pcie configuration space

        //checks if this device exists
        if (device->general.header.vendor_id == 0xFFFF) {
            break;
        }

        pci_header_type_t type = (device->general.header.header_type & 0x7F);
        pci_tree_bus_t *current = pci_tree_get_bus_by_id(bus);                             //the bus we're scanning the device on
        assert_true(current != null);

        if (type == general_device) {
            pci_general_dev_t *dev = &device->general;
            if (!pci_init_device(dev)) {return false;}
            pci_tree_device_t *new_device = pci_tree_create_device(dev, current);          //create a new device node object

            if (!pci_tree_install_device(new_device, current)) {                           //installs it into the pci tree
                return false;
            }
        } else if (type == pci_to_pci_bridge) {
            pci_pci_bridge_t *bridge = &device->bridge;
            pci_tree_bus_t *new_bus = pci_tree_create_bus(bridge->secondary_bus_number);   //create a new bus node object
            
            if (!pci_tree_install_bus(current, new_bus, bridge)) {                         //installs it into the pci tree
                return false;
            }

            pcie_scan_bus(config_base, bridge->secondary_bus_number);                      //recursively scan the newly discovered bus
        } else {
            continue;                                                                      //unsupported header type
        }

        bool multifunction = device->general.header.header_type >> 7;                      //msb of header type set if this is a multifunction device

        if (!multifunction && fnc == 0) {
            break;
        }
    }

    return true;
}