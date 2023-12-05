#pragma once
#include <include/types.h>
#include <io/include/pci.h>
#include <drv/ahci/include/ahci.h>

#define AHCI_COMMAND_LIST_SIZE 

bool ahci_init(pci_general_dev_t *dev);
bool ahci_search_and_add_devices(ahci_hba_memory_t *hba_mem);
bool ahci_init_device(volatile ahci_hba_port_t *port);