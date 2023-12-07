#include <include/types.h>
#include <drv/ahci/include/ahci_setup.h>
#include <io/include/pci.h>
#include <io/include/devs.h>
#include <mm/include/obj_alloc.h>
#include <mm/include/memory_manager.h>
#include <drv/ahci/include/ahci.h>
#include <include/mem.h>
#include <drv/ide/include/ide.h>
#include <mm/include/paging.h>

pool_t ahci_devices_pool_id;
uint32_t ahci_devices_pool_last_id;

bool ahci_init(pci_general_dev_t *dev) {
    if (!dev || (pci_dev_type_t) dev->header.class_code != mass_storage || (mass_storage_subclass_t) dev->header.subclass != sata) {
        return false;
    }

    //AHCI bar
    void *abar = (void *)(uint64_t) dev->bar5;

    if (!abar) {
        return false;
    }

    if (!ahci_search_and_add_devices((ahci_hba_memory_t *) abar)) {
        return false;
    }
    
    return true;
}

bool ahci_search_and_add_devices(ahci_hba_memory_t *hba_mem) {
    if (!hba_mem) {
        return false;
    }
    
    for (uint8_t i = 0; i < 32; i++) {
        if (hba_mem->port_impl >> i & 1) {
            if (!ahci_init_device(&hba_mem->ports[i])) {
                return false;
            }

            ahci_device_t dev = {
                .ctrl = hba_mem,
                .port = &hba_mem->ports[i]
            };

            if (obj_pool_put(ahci_devices_pool_id, (void *) &dev, ahci_devices_pool_last_id)) {
                ahci_devices_pool_last_id++;
            } else {
                return false;
            }
        }
    }

    return true;
}

/* initialize a sata device */
bool ahci_init_device(volatile ahci_hba_port_t *port) {
    if (!port) {
        return false;
    }

    uint8_t ipm = (port->sata_status >> 8) & 0x0F;
    uint8_t det = port->sata_status & 0x0F;

    //check that this is a SATA device
    if (det != 0x03 || ipm != 0x01 || (ahci_device_type_t) port->signature != ahci_ata) {
        return true;
    }

    ahci_comm_header_t *comm_list = (ahci_comm_header_t *)((uint64_t) port->clb_hi << 32 | port->clb_lo);
    
    for (uint8_t i = 0; i < 32; i++) {
        ahci_comm_header_t *cl_entry = comm_list + i;
        ahci_hba_command_table_t *comm_table = (ahci_hba_command_table_t *)((uint64_t) cl_entry->comm_table_hi << 32 | cl_entry->comm_table_lo);
        printf("command table %d at 0x%X\n", i, comm_table);
    }

    return true;
}