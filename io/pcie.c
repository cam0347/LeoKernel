#include <io/include/pci.h>
#include <io/include/pcie.h>
#include <include/types.h>
#include <io/include/port_io.h>
#include <include/mem.h>
#include <acpi/include/acpi.h>
#include <mm/include/obj_alloc.h>

extern pool_t pci_dev_pool_id; //defined in pci.c
extern uint32_t pci_dev_pool_last_ind = 0; //defined in pci.c

/* read a device descriptor from the pcie memory configuration space */
bool pcie_config_read_entry(void *conf_base, uint8_t bus_start, uint8_t bus, uint8_t dev, uint8_t fnc, pci_device_t *out) {
    pci_device_t *obj = (pci_device_t *)(conf_base + (((uint64_t)(bus - bus_start) << 20) | ((uint64_t) dev << 15) | ((uint64_t) fnc << 12)));

    memcpy(out, obj, sizeof(pci_device_t));

    if ((obj->general.header.header_type & 0x7F) == general_device) {
        pci_general_device_reverse_endianess(&out->general);
    } else if ((obj->general.header.header_type & 0x7F) == pci_to_pci_bridge) {
        pci_ptp_bridge_reverse_endianess(&out->bridge);
    } else {
        return false;
    }

    if (out->general.header.vendor_id == 0xFFFF) {
        return false;
    }

    return true;
}

/* enumerate devices using pcie configuration mechanism */
void enum_pcie() {
    acpi_mcfg_t *mcfg = (acpi_mcfg_t *) acpi_locate_table("MCFG", 0);
    
    if (!mcfg) {
        return;
    }

    uint32_t n_entries = (mcfg->h.length - sizeof(acpi_table_header) - 8) / sizeof(acpi_mcfg_entry_t);

    for (uint32_t i = 0; i < n_entries; i++) {
        acpi_mcfg_entry_t *entry = &mcfg->entries[i];

        for (uint16_t bus = entry->start_bus; bus < entry->end_bus; bus++) {
            for (uint8_t dev = 0; dev < 32; dev++) {
                pcie_scan_device((void *) entry->base, entry->start_bus, bus, dev);
            }
        }
    }
}

/* scans a device functions and adds them to the devices pool */
void pcie_scan_device(void *conf_base, uint8_t bus_start, uint8_t bus, uint8_t dev) {
    for (uint8_t fnc = 0; fnc < 8; fnc++) {
        pci_device_t devfnc;
        
        if (!pcie_config_read_entry(conf_base, bus_start, bus, dev, fnc, &devfnc)) {
            break; //entry non valida o non presente
        }

        obj_pool_put(pci_dev_pool_id, (void *) &devfnc, pci_dev_pool_last_ind++);
        bool multifunction = devfnc.general.header.header_type >> 7;

        if (!multifunction && fnc == 0) {
            break;
        }
    }
}