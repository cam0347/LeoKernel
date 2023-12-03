/*
IDE driver.
This file contains the functions to actually use the IDE devices.
*/

#include <include/types.h>
#include <drv/ide/include/ide.h>
#include <drv/ide/include/ide_pio.h>
#include <include/mem.h>

/* reads some sectors using pio mode */
bool ide_read(ide_device_t *dev, uint64_t address, uint32_t sectors, void *buffer) {
    if (!dev || !buffer || sectors == 0 || !dev->bus) {
        return false;
    }

    if (!ide_check_type(dev)) {
        return false;
    }

    ide_select_drive(dev->bus, dev->drive);

    if (!ide_init_transaction(dev, address, sectors)) {
        return false;
    }

    if (dev->addr_mode == chs || dev->addr_mode == lba28) {
        ide_command(dev->bus, IDE_COMM_READ_PIO);
    } else if (dev->addr_mode == lba48) {
        ide_command(dev->bus, IDE_COMM_READ_PIO_EXT);
    } else {
        return false;
    }

    for (uint32_t i = 0; i < sectors * 256; i++) {
        ide_poll(dev->bus);

        if (ide_check_error(dev->bus)) {
            memclear(buffer, sectors * 512);
            return false;
        }

        *((uint16_t *) buffer + i) = ide_read_reg(dev->bus->io_bar, IDE_BAR_OFFSET_DATA_REG);
    }

    return true;
}

bool ide_write(ide_device_t *dev, uint64_t address, uint32_t sectors, void *data) {
    if (!dev || !data || sectors == 0 || !dev->bus) {
        return false;
    }

    if (!ide_check_type(dev)) {
        return false;
    }

    ide_select_drive(dev->bus, dev->drive);

    if (!ide_init_transaction(dev, address, sectors)) {
        return false;
    }

    uint8_t flush_command;

    if (dev->addr_mode == chs || dev->addr_mode == lba28) {
        ide_command(dev->bus, IDE_COMM_WRITE_PIO);
        flush_command = IDE_COMM_CACHE_FLUSH;
    } else if (dev->addr_mode == lba48) {
        ide_command(dev->bus, IDE_COMM_WRITE_PIO_EXT);
        flush_command = IDE_COMM_CACHE_FLUSH_EXT;
    } else {
        return false;
    }

    for (uint32_t i = 0; i < sectors * 256; i++) {
        ide_poll(dev->bus);
        ide_write_reg(dev->bus->io_bar, IDE_BAR_OFFSET_DATA_REG, *((uint16_t *) data + i));
        ide_command(dev->bus, flush_command);
    }

    return true;
}

/* this driver doesn't support packet interface */
bool ide_check_type(ide_device_t *dev) {
    ide_type_t type = dev->ide_type;
    return type == ide_pata || type == ide_sata;
}