#pragma once
#include <include/types.h>
#include <io/include/pci.h>

struct pci_tree_dev_node {
    pci_general_dev_t dev;
    struct pci_tree_dev_node *next;
};

struct pci_tree_bus_node {
    pci_pci_bridge_t bus;
    struct pci_tree_bus_node *sub;
};

typedef struct pci_tree_dev_node pci_tree_dev_node_t;
typedef struct pci_tree_bus_node pci_tree_bus_node_t;