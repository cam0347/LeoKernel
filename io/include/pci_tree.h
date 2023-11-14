#pragma once
#include <include/types.h>
#include <io/include/pci.h>
#define PCI_BUS_INITIAL_NUMBER 0

struct pci_tree_dev_node {
    pci_general_dev_t dev;
    struct pci_tree_bus_node *bus;
    struct pci_tree_dev_node *next;
};

struct pci_tree_bus_node {
    pci_pci_bridge_t bus;
    uint8_t bus_num;
    struct pci_tree_bus_node *sub;
    struct pci_tree_bus_node *next;
    struct pci_tree_dev_node *devs;
};

typedef struct pci_tree_dev_node pci_tree_dev_node_t;
typedef struct pci_tree_bus_node pci_tree_bus_node_t;

void init_pci_tree(void);
bool pci_tree_append_child(pci_tree_bus_node_t *bus, pci_general_dev_t *child);
bool pci_tree_append_bus(pci_tree_bus_node_t *master, pci_pci_bridge_t *slave);