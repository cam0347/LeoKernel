#include <include/types.h>
#include <io/include/pci.h>
#include <io/include/pcie.h>
#include <drv/ide/include/ide.h>
#include <drv/ahci/include/ahci.h>
#include <mm/include/obj_alloc.h>
#include <io/include/devs.h>
#include <io/include/pci_tree.h>

bool pcie_supported = false;
extern pool_t ide_controllers_pool_id; //defined in ide_setup.c
extern pool_t ahci_controllers_pool_id; //defined in ahci_setup.c

bool init_pci() {
    if (!init_pci_tree()) {
        return false;
    }

    //create some pools
    if (!create_obj_pool(&ide_controllers_pool_id, sizeof(ide_controller_t), 8, manual)) {return false;}
    if (!create_obj_pool(&ahci_controllers_pool_id, sizeof(ahci_controller_t), 8, manual)) {return false;}

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
    }
}