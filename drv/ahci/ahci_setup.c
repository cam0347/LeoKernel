#include <include/types.h>
#include <drv/ahci/include/ahci_setup.h>
#include <io/include/pci.h>
#include <io/include/devs.h>
#include <mm/include/obj_alloc.h>
#include <drv/ahci/include/ahci.h>

pool_t ahci_controllers_pool_id;
uint32_t ahci_controllers_pool_last_id;

bool ahci_init(pci_general_dev_t *dev) {
    if (!dev || (pci_dev_type_t) dev->header.class_code != mass_storage || (mass_storage_subclass_t) dev->header.subclass != sata) {
        return false;
    }

    

    return true;
}