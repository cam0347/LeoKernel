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

    /*
    allocate a page for this device's command list (1K) and
    FIS base (512 bytes).
    */
    void *device_memory = kalloc_page(1);
    void *physical = get_physical_address(device_memory);

    if (!device_memory || physical == TRANSLATION_UNKNOWN) {
        return false;
    }
    
    void *clb = physical;
    void *fb = clb + 1024; //the command list takes 32 * 32 bytes

    port->clb_lo = (uint32_t)((uint64_t) clb & 0xFFFFFFFF);
    port->clb_hi = (uint32_t)((uint64_t) clb >> 32 & 0xFFFFFFFF);
    port->fis_base_lo = (uint32_t)((uint64_t) fb & 0xFFFFFFFF);
    port->fis_base_hi = (uint32_t)((uint64_t) fb >> 32 & 0xFFFFFFFF);

    return true;
}