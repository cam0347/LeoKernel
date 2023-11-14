#pragma once
#include <include/types.h>
#include <io/include/pci.h>

bool pcie_config_read_entry(void *conf_base, uint8_t bus_start, uint8_t bus, uint8_t dev, uint8_t fnc, pci_device_t *out);
void enum_pcie();
void pcie_scan_device(void *conf_base, uint8_t bus_start, uint8_t bus, uint8_t dev);