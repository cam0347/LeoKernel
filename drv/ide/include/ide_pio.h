#pragma once
#include <include/types.h>
#include <drv/ide/include/ide.h>

bool ide_read(ide_device_t *dev, uint64_t address, uint32_t sectors, void *buffer);
bool ide_write(ide_device_t *dev, uint64_t address, uint32_t sectors, void *data);
bool ide_check_type(ide_device_t *dev);