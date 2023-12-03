#pragma once
#include <include/types.h>
#include <drv/ide/include/ide.h>

typedef uint32_t ide_drive_id;

bool ide_read_wrapper(ide_drive_id ide_id, uint64_t address, uint64_t sectors, void *buffer);
bool ide_write_wrapper(ide_drive_id ide_id, uint64_t address, uint64_t sectors, void *data);
bool ide_check_magic(ide_device_t *dev);