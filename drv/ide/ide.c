#include <include/types.h>
#include <drv/ide/include/ide.h>
#include <io/include/port_io.h>

uint16_t ide_read_reg(uint32_t bar, uint8_t reg) {
    return inw(bar + reg);
}

void ide_write_reg(uint32_t bar, uint8_t reg, uint16_t data) {
    outw(bar + reg, data);
}

void ide_select_drive(ide_bus_t *bus, ide_drive_select_t drive) {
    if (!bus) {
        return;
    }

    if ((uint8_t) ide_read_reg(bus->io_bar, IDE_BAR_OFFSET_DRIVE_HEAD) == drive) {
        return; //that drive was already selected
    }

    while(ide_status(bus) >> 7 & 1); //waits for busy bit to clear
    ide_write_reg(bus->io_bar, IDE_BAR_OFFSET_DRIVE_HEAD, drive & 0xFF);
    ide_wait(bus); //give the drive some time
}

uint8_t ide_status(ide_bus_t *bus) {
    return (uint8_t) ide_read_reg(bus->io_bar, IDE_BAR_OFFSET_STATUS);
}

void ide_wait(ide_bus_t *bus) {
    for (uint8_t i = 0; i < 15; i++) {
        ide_status(bus);
    }
}

void ide_poll(ide_bus_t *bus) {
    //waits for bit 7 to clear and bit 3 to set or for an error to raise
    while(((ide_status(bus) >> 7 & 1) || !(ide_status(bus) >> 3 & 1)) && !ide_check_error(bus));
}

uint8_t ide_error(ide_bus_t *bus) {
    return (uint8_t) ide_read_reg(bus->io_bar, IDE_BAR_OFFSET_ERROR_REG);
}

/* if the status bit 0 or 5 are set that means there's an error */
bool ide_check_error(ide_bus_t *bus) {
    uint8_t status = ide_status(bus);
    return (status & 1) || (status >> 5 & 1);
}

/* resets disks on a bus (can't be reset individually) */
void ide_reset(ide_bus_t *bus) {
    uint8_t old = (uint8_t) ide_read_reg(bus->ctrl_bar, 0);
    ide_write_reg(bus->ctrl_bar, IDE_BAR_OFFSET_DEV_CTRL, old | 4); //set bit 2
    ide_write_reg(bus->ctrl_bar, IDE_BAR_OFFSET_DEV_CTRL, old); //resets bit 2
}

void ide_command(ide_bus_t *bus, uint8_t command) {
    if (!bus) {
        return;
    }

    while(ide_status(bus) >> 7 & 1); //waits for busy bit to reset
    ide_write_reg(bus->io_bar, IDE_BAR_OFFSET_COMMAND, command);
}

bool ide_init_transaction(ide_device_t *dev, uint64_t address, uint64_t sectors) {
    if (!dev || sectors == 0 || !dev->bus) {
        return false;
    }

    uint8_t selection, drive_select;
    ide_bus_t *bus = dev->bus;

    if (sectors > dev->sectors) {
        return false;
    }

    //write address and sector count
    switch(dev->addr_mode) {
        case chs:
            selection = dev->drive | (address >> 24 & 0x0F);                               //drive select and head number (address byte 3)
            ide_write_reg(bus->io_bar, IDE_BAR_OFFSET_DRIVE_HEAD, selection);
            ide_write_reg(bus->io_bar, IDE_BAR_OFFSET_SECTOR_COUNT, sectors);
            ide_write_reg(bus->io_bar, IDE_BAR_OFFSET_LBA_LO, address & 0xFF);             //sector number (address byte 0)
            ide_write_reg(bus->io_bar, IDE_BAR_OFFSET_LBA_MID, address >> 8 & 0xFF);       //cylinder low (address byte 1)
            ide_write_reg(bus->io_bar, IDE_BAR_OFFSET_LBA_HI, address >> 16 & 0xFF);       //cylinder high (address byte 2)
            break;

        case lba28:
            selection = dev->drive | (1 << 6) | (address >> 24 & 0x0F);                    //drive select and highest 4 bits of LBA28 address
            ide_write_reg(bus->io_bar, IDE_BAR_OFFSET_DRIVE_HEAD, selection);
            ide_write_reg(bus->io_bar, IDE_BAR_OFFSET_SECTOR_COUNT, (uint8_t) sectors);    //sector number
            ide_write_reg(bus->io_bar, IDE_BAR_OFFSET_LBA_LO, address & 0xFF);             //LBA28 low (byte 0)
            ide_write_reg(bus->io_bar, IDE_BAR_OFFSET_LBA_MID, address >> 8 & 0xFF);       //LBA28 mid (byte 1)
            ide_write_reg(bus->io_bar, IDE_BAR_OFFSET_LBA_HI, address >> 16 & 0xFF);       //LBA28 (byte 2)
            break;

        case lba48:
            selection = dev->drive | (1 << 6);
            ide_write_reg(bus->io_bar, IDE_BAR_OFFSET_DRIVE_HEAD, selection);
            ide_write_reg(bus->io_bar, IDE_BAR_OFFSET_SECTOR_COUNT, sectors >> 8 & 0xFF);  //sector count high (byte 1)
            ide_write_reg(bus->io_bar, IDE_BAR_OFFSET_LBA_LO, address >> 24 & 0xFF);       //LBA48 byte 3
            ide_write_reg(bus->io_bar, IDE_BAR_OFFSET_LBA_MID, address >> 32 & 0xFF);      //LBA48 byte 4
            ide_write_reg(bus->io_bar, IDE_BAR_OFFSET_LBA_HI, address >> 40 & 0xFF);       //LBA48 byte 5
            ide_write_reg(bus->io_bar, IDE_BAR_OFFSET_SECTOR_COUNT, sectors & 0xFF);       //sector count low (byte 0)
            ide_write_reg(bus->io_bar, IDE_BAR_OFFSET_LBA_LO, address & 0xFF);             //LBA48 byte 0
            ide_write_reg(bus->io_bar, IDE_BAR_OFFSET_LBA_MID, address >> 8 & 0xFF);       //LBA48 byte 1
            ide_write_reg(bus->io_bar, IDE_BAR_OFFSET_LBA_HI, address >> 16 & 0xFF);       //LBA48 byte 2
            break;

        default:
            return false;
    }
    
    return true;
}