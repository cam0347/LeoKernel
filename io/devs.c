/*
This files contains the functions to configure the peripherals found scanning the pci bus.
For each type of peripheral there's a function.
*/

#include <include/types.h>
#include <io/include/devs.h>
#include <io/include/pci.h>

/* drivers */
#include <drv/ide/include/ide_setup.h>

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
            break;

        case nvm:
            break;

        default:
            return false;
            break;
    }

    return true;
}