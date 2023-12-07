/*
This files contains the functions to configure the peripherals found scanning the pci bus.
For each type of peripheral there's a function.
*/

#include <include/types.h>
#include <io/include/devs.h>
#include <io/include/pci.h>
#include <mm/include/obj_alloc.h>

/* drivers */
#include <drv/ide/include/ide_setup.h>
#include <drv/ahci/include/ahci_setup.h>

pool_t mass_storage_pool_id;
uint32_t mass_storage_last_ind = 0;

/* configure a mass storage device based on its type */
bool devs_config_mass_storage(pci_general_dev_t *dev) {
    if (!dev) {
        return false;
    }

    switch((mass_storage_subclass_t) dev->header.subclass) {
        case ide:
            return ide_init(dev);
            break;

        case ata:
            break;

        case sata:
            return ahci_init(dev);
            break;

        case nvm:
            break;

        default:
            return false;
            break;
    }

    return true;
}