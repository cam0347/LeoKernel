#pragma once
#include <include/types.h>
#include <io/include/pci.h>
#define PCI_INITIAL_BUS 0

typedef struct pci_tree_dev_node {
    pci_general_dev_t *dev;                           //pointer to the actual data structure
    struct pci_tree_bus_node *bus;                    //pointer to the bus this devices is attached to
    struct pci_tree_dev_node *next;                   //next device on this bus
} pci_tree_device_t;

typedef struct pci_tree_bus_node {
    uint8_t id;                                       //bus id
    struct pci_tree_bridge_node *bridges;             //pointer to the bridges to other buses
    struct pci_tree_dev_node *devs;                   //pointer to a list of devices
} pci_tree_bus_t;

typedef struct pci_tree_bridge_node {
    pci_tree_bus_t *primary;                          //pointer to primary (upstream) bus
    pci_tree_bus_t *secondary;                        //pointer to secondary (downstream) bus
    pci_pci_bridge_t *bridge;                         //pointer to the actual data structure
    struct pci_tree_bridge_node *next;                //pointer to the next bridge (sibling bridges share the upstream bus)
} pci_tree_bridge_t;

bool init_pci_tree(void);
void pci_tree_print_indent(uint8_t indent);
void pci_tree_print_bus(pci_tree_bus_t *bus, uint8_t indent);
void pci_tree_print(void);

pci_tree_bus_t *pci_tree_get_bus_by_id(uint8_t id);
pci_tree_bus_t *pci_tree_create_bus(uint8_t id);
bool pci_tree_install_bus(pci_tree_bus_t *primary, pci_tree_bus_t *secondary, pci_pci_bridge_t *bridge);

pci_tree_device_t *pci_tree_create_device(pci_general_dev_t *dev, pci_tree_bus_t *bus);
bool pci_tree_install_device(pci_tree_device_t *dev, pci_tree_bus_t *bus);