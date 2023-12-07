#include <include/types.h>
#include <io/include/pci.h>
#include <io/include/pcie.h>
#include <drv/ide/include/ide.h>
#include <drv/ahci/include/ahci.h>
#include <mm/include/obj_alloc.h>
#include <io/include/devs.h>
#include <io/include/pci_tree.h>

extern pool_t ide_devices_pool_id; //defined in ide_setup.c
extern pool_t ahci_devices_pool_id; //defined in ahci_setup.c
extern pool_t mass_storage_pool_id; //defined in devs.c

bool init_pci() {
    if (!init_pci_tree()) {
        return false;
    }

    //create some pools
    if (!create_obj_pool(&mass_storage_pool_id, sizeof(ms_dev_t), 8, manual)) {return false;}          //mass storage devices
    if (!create_obj_pool(&ide_devices_pool_id, sizeof(ide_device_t), 16, manual)) {return false;}      //IDE devices
    if (!create_obj_pool(&ahci_devices_pool_id, sizeof(ahci_device_t), 128, manual)) {return false;}   //AHCI devices

    return enum_pcie();
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

        default:
            return true;
    }

    return false;
}