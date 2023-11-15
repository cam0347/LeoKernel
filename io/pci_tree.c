#include <include/types.h>
#include <io/include/pci_tree.h>
#include <include/string.h>
#include <mm/include/kmalloc.h>
#include <include/mem.h>
#include <include/assert.h>
#include <tty/include/tty.h>

pci_tree_bus_t *pci_tree_root;

/* initializes the pci device tree by creating the first bus */
bool init_pci_tree(void) {
    if (!(pci_tree_root = pci_tree_create_bus(PCI_INITIAL_BUS))) {
        return false;
    }

    return true;
}

/* create a bus node in the pci tree */
pci_tree_bus_t *pci_tree_create_bus(uint8_t id) {
    pci_tree_bus_t *node;

    if (!(node = kmalloc(sizeof(pci_tree_bus_t)))) {
        return null;
    }

    node->id = id;
    node->devs = null;
    node->bridges = null;

    return node;
}

/* returns the tree node associated to the specified bus id */
pci_tree_bus_t *pci_tree_get_bus_by_id(uint8_t id) {
    pci_tree_bus_t *current = pci_tree_root;

    while(current->id != id) {
        if (!current->bridges) {
            return null;
        }

        pci_tree_bridge_t *bridge = current->bridges;

        do {
            if (id >= bridge->primary->id && id <= bridge->bridge->subordinate_bus_number) {
                current = bridge->secondary;
                break;
            } else {
                bridge = bridge->next;
            }
        } while(bridge);

        if (!bridge) {
            return null;
        }
    }

    return current;
}

/* create a device node in the pci tree */
pci_tree_device_t *pci_tree_create_device(pci_general_dev_t *dev, pci_tree_bus_t *bus) {
    pci_tree_device_t *node;

    if (!(node = kmalloc(sizeof(pci_tree_device_t)))) {
        return null;
    }

    node->bus = bus;
    node->dev = dev;
    node->next = null;

    return node;
}

/* appends a device to a bus device list */
bool pci_tree_install_device(pci_tree_device_t *dev, pci_tree_bus_t *bus) {
    if (!dev || !bus) {
        return false;
    }

    dev->bus = bus;
    dev->next = null;

    if (!bus->devs) {
        bus->devs = dev;
        return true;
    }

    pci_tree_device_t *curr = bus->devs;

    while(curr->next) {
        curr = curr->next;
    }

    curr->next = dev;
    return true;
}

/* appends a secondary bus to a primary bus using a bridge */
bool pci_tree_install_bus(pci_tree_bus_t *primary, pci_tree_bus_t *secondary, pci_pci_bridge_t *bridge) {
    if (!primary || !secondary) {
        return false;
    }

    pci_tree_bridge_t *_bridge;

    if (!(_bridge = kmalloc(sizeof(pci_tree_bridge_t)))) {
        return false;
    }

    _bridge->bridge = bridge;
    _bridge->primary = primary;
    _bridge->secondary = secondary;
    _bridge->next = null;

    if (!primary->bridges) {
        primary->bridges = _bridge;
        return true;
    }

    pci_tree_bridge_t *curr = primary->bridges;

    while(curr->next) {
        curr = curr->next;
    }

    curr->next = _bridge;
    return true;
}

void pci_tree_print(void) {
    if (!pci_tree_root->devs) {
        printf("no devices\n");
        return;
    }

    pci_tree_device_t *device = pci_tree_root->devs;

    do {
        printf("(CC %d, SC %d)\n", device->dev->header.class_code, device->dev->header.subclass);
        device = device->next;
    } while (device);

    if (pci_tree_root->bridges) {
        printf("further buses\n");
    } else {
        printf("no subordinate buses\n");
    }
}