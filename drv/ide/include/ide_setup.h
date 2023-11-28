#pragma once
#include <include/types.h>
#include <io/include/pci.h>
#include <drv/ide/include/ide.h>

bool ide_init(pci_general_dev_t *dev);
void ide_save_device_info(ide_device_t *dev, uint16_t *info);
bool ide_identify_drive(ide_bus_t *bus, ide_drive_select_t drive, void *info);
void ide_save_device_ide_type(ide_bus_t *bus, ide_drive_select_t select);
bool ide_set_channels_mode(pci_general_dev_t *dev);