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

/* create a bus node */
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

/* returns the tree node associated to the bus with the specified id */
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

/* create a device node */
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

/* installs the device into the pci tree */
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

/* installs the bus into the pci tree */
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

inline void pci_tree_print_indent(uint8_t indent) {
    for (uint8_t i = 0; i < indent; i++) {
        printf("    ");
    }
}

void pci_tree_print(void) {
    pci_tree_print_bus(pci_tree_root, 0);
}

void pci_tree_print_bus(pci_tree_bus_t *bus, uint8_t indent) {
    if (!bus->devs) {
        pci_tree_print_indent(indent);
        printf("no devices\n");
        return;
    }

    pci_tree_print_indent(indent);
    printf("BUS #%d\n", bus->id);
    pci_tree_device_t *device = bus->devs;

    do {
        pci_tree_print_indent(indent);
        printf("class code %d, subclass %d, INT%c#\n",
            device->dev->header.class_code,
            device->dev->header.subclass,
            device->dev->int_pin != 0 ? 'A' - 1 + device->dev->int_pin : 'F');
        device = device->next;
    } while (device);

    if (!bus->bridges) {
        return;
    }

    pci_tree_bridge_t *bridge = bus->bridges;

    do {
        pci_tree_print_bus(bridge->secondary, indent + 1);
        bridge = bridge->next;
    } while (bridge);
}