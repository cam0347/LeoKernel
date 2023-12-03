/*
IDE driver.
This file contains the wrapper functions to use an IDE device.
These functions build a minimal abstraction layer on the driver.
Here, the devices are referenced by id.
*/

#include <include/types.h>
#include <drv/ide/include/ide.h>
#include <drv/ide/include/ide_pio.h>
#include <mm/include/obj_alloc.h>
#include <drv/ide/include/ide_wrapper.h>

extern pool_t ide_devices_pool_id; //defined in ide_setup.c

bool ide_read_wrapper(ide_drive_id ide_id, uint64_t address, uint64_t sectors, void *buffer) {
    if (!buffer) {
        return false;
    }

    if (sectors == 0) {
        return true;
    }

    ide_device_t *dev;

    if (!obj_pool_get(ide_devices_pool_id, (void *) &dev, (uint32_t) ide_id)) {
        return false;
    }

    if (!ide_check_magic(dev)) {
        return false;
    }

    return ide_read(dev, address, sectors, buffer);
}

bool ide_write_wrapper(ide_drive_id ide_id, uint64_t address, uint64_t sectors, void *data) {
    if (!data) {
        return false;
    }

    if (sectors == 0) {
        return true;
    }

    ide_device_t *dev;

    if (!obj_pool_get(ide_devices_pool_id, (void *) &dev, (uint32_t) ide_id)) {
        return false;
    }

    if (!ide_check_magic(dev)) {
        return false;
    }

    return ide_write(dev, address, sectors, data);
}

bool ide_check_magic(ide_device_t *dev) {
    return dev->magic == IDE_DEVICE_MAGIC;
}