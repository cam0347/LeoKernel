#include <include/types.h>
#include <io/include/pci_tree.h>
#include <include/string.h>
#include <mm/include/kmalloc.h>
#include <include/mem.h>
#include <include/assert.h>

pci_tree_bus_node_t pci_tree_root;
uint8_t last_bus_id = PCI_BUS_INITIAL_NUMBER;

void init_pci_tree(void) {
    memclear((void *) &pci_tree_root, sizeof(pci_tree_bus_node_t));
    pci_tree_root.bus_num = last_bus_id++;
    pci_tree_root.devs = null;
    pci_tree_root.sub = null;
}

/* appends a child device to a bus */
bool pci_tree_append_child(pci_tree_bus_node_t *bus, pci_general_dev_t *child) {
    assert_true(bus != null && child != null);

    //create tree node
    pci_tree_dev_node_t *node;
    
    if (!(node = kmalloc(sizeof(pci_tree_dev_node_t)))) {
        return false;
    }

    node->bus = bus;
    node->dev = *child; //copy the pci_general_dev_t struct into the node
    node->next = null; //last child, null next pointer

    //if this bus has no children, set this as first
    if (!bus->devs) {
        bus->devs = child;
        return true;
    }

    pci_tree_dev_node_t *current = bus->devs; //get the first child

    while(current->next) {
        current = current->next;
    }

    current->next = node;
    return true;
}

/*
appends a slave bus to a master bus.
this funcion assign a unique bus number to the new bus.
*/
bool pci_tree_append_bus(pci_tree_bus_node_t *master, pci_pci_bridge_t *slave) {
    assert_true(master != null && slave != null);

    pci_tree_bus_node_t *node;

    if (!(node = kmalloc(sizeof(pci_tree_bus_node_t)))) {
        return false;
    }

    node->next = null;
    node->bus = *slave;
    node->devs = null;
    node->sub = null;
    node->bus_num = last_bus_id++; //assign bus number

    if (!master->sub) {
        master->sub = slave;
        return true;
    }

    pci_tree_bus_node_t *current = master->sub;

    while(current->next) {
        current = current->next;
    }

    current->next = slave;
    return true;
}