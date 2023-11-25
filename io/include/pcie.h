#pragma once
#include <include/types.h>
#include <io/include/pci.h>

//returns the address of the specified function in the pcie memory mapped space
#define PCIE_FNC_ADDRESS(config, bus, dev, fnc) ((pci_device_t *)(config + (((uint64_t) bus << 20) | ((uint64_t) dev << 15) | ((uint64_t) fnc << 12))))

bool pcie_config_read_entry(void *conf_base, uint8_t bus, uint8_t dev, uint8_t fnc, pci_device_t *out);
bool enum_pcie();
bool pcie_scan_bus(void *config_base, uint8_t bus);
bool pcie_scan_device(void *config_base, uint8_t bus, uint8_t dev);