#pragma once
#include <include/types.h>
#include <drv/ide/include/ide.h>
#include <io/include/pci.h>

bool ide_init(pci_general_dev_t *pci_dev);
uint8_t ide_bus_test_and_add_drives(ide_bus_t *bus);
bool ide_identify(ide_device_t *dev, void *info);
void ide_save_device_info(ide_device_t *dev, uint16_t *info);
void ide_save_device_ide_type(ide_device_t *dev);
bool ide_set_bus_mode(pci_general_dev_t *pci_dev, ide_bus_select_t bus, ide_channel_mode_t mode);
void ide_bus_init(ide_bus_t *bus, uint32_t io, uint32_t ctrl, bool dma, ide_prd_t *prdt);