#include <include/types.h>
#include <drv/ide/include/ide.h>
#include <drv/ide/include/ide_io.h>
#include <io/include/port_io.h>

void ide_wait(ide_bus_t *bus) {
    for (uint8_t i = 0; i < 15; i++) {
        ide_status(bus);
    }
}

void ide_poll(ide_bus_t *bus) {
    //waits for bit 7 to clear and bit 3 to set or for an error to raise
    while(((ide_status(bus) >> 7 & 1) || !(ide_status(bus) >> 3 & 1)) && !ide_check_error(bus));
}

/* if the status bit 0 or 5 are set that means there's an error */
bool ide_check_error(ide_bus_t *bus) {
    uint8_t status = ide_status(bus);
    return (status & 1) || (status >> 5 & 1);
}

/* read a register */
uint16_t ide_read(uint32_t bar, uint8_t reg) {
    return inw(bar + reg);
}

/* write a register */
void ide_write(uint32_t bar, uint8_t reg, uint16_t data) {
    outw(bar + reg, data);
}

/*
prepares a drive for a transaction (read or write).
In the case of a CHS addressing mode, the format of the address parameter is:
    - byte 0: sector number
    - byte 1: cylinder low
    - byte 2: cylinder high
    - byte 3: head (actually a nybble)

In the case of a LBA28/LBA48 addressing mode, the address parameter is the LBA address.
*/
bool ide_init_transaction(ide_bus_t *bus, uint64_t address, uint16_t sectors) {
    if (!bus || sectors == 0) {
        return false;
    }

    //get the selected drive
    ide_device_t *selected = bus->selected == master ? &bus->master : &bus->slave;
    uint8_t selection, drive_select;

    if (sectors > selected->sectors) {
        return false;
    }

    //write address and sector count
    switch(selected->addr_mode) {
        case chs:
            selection = bus->selected | (address >> 24 & 0x0F);                    //drive select and head number (address byte 3)
            ide_write(bus->io, IDE_BAR_OFFSET_DRIVE_HEAD, selection);
            ide_write(bus->io, IDE_BAR_OFFSET_SECTOR_COUNT, sectors);
            ide_write(bus->io, IDE_BAR_OFFSET_LBA_LO, address & 0xFF);             //sector number (address byte 0)
            ide_write(bus->io, IDE_BAR_OFFSET_LBA_MID, address >> 8 & 0xFF);       //cylinder low (address byte 1)
            ide_write(bus->io, IDE_BAR_OFFSET_LBA_HI, address >> 16 & 0xFF);       //cylinder high (address byte 2)
            break;

        case lba28:
            selection = bus->selected | (1 << 6) | (address >> 24 & 0x0F);         //drive select and highest 4 bits of LBA28 address
            ide_write(bus->io, IDE_BAR_OFFSET_DRIVE_HEAD, selection);
            ide_write(bus->io, IDE_BAR_OFFSET_SECTOR_COUNT, (uint8_t) sectors);    //sector number
            ide_write(bus->io, IDE_BAR_OFFSET_LBA_LO, address & 0xFF);             //LBA28 low (byte 0)
            ide_write(bus->io, IDE_BAR_OFFSET_LBA_MID, address >> 8 & 0xFF);       //LBA28 mid (byte 1)
            ide_write(bus->io, IDE_BAR_OFFSET_LBA_HI, address >> 16 & 0xFF);       //LBA28 (byte 2)
            break;

        case lba48:
            ide_write(bus->io, IDE_BAR_OFFSET_DRIVE_HEAD, (uint8_t)(bus->selected | (1 << 6)));
            ide_write(bus->io, IDE_BAR_OFFSET_SECTOR_COUNT, sectors >> 8 & 0xFF);  //sector count high (byte 1)
            ide_write(bus->io, IDE_BAR_OFFSET_LBA_LO, address >> 24 & 0xFF);       //LBA48 byte 3
            ide_write(bus->io, IDE_BAR_OFFSET_LBA_MID, address >> 32 & 0xFF);      //LBA48 byte 4
            ide_write(bus->io, IDE_BAR_OFFSET_LBA_HI, address >> 40 & 0xFF);       //LBA48 byte 5
            ide_write(bus->io, IDE_BAR_OFFSET_SECTOR_COUNT, sectors & 0xFF);       //sector count low (byte 0)
            ide_write(bus->io, IDE_BAR_OFFSET_LBA_LO, address & 0xFF);             //LBA48 byte 0
            ide_write(bus->io, IDE_BAR_OFFSET_LBA_MID, address >> 8 & 0xFF);       //LBA48 byte 1
            ide_write(bus->io, IDE_BAR_OFFSET_LBA_HI, address >> 16 & 0xFF);       //LBA48 byte 2
            break;

        default:
            return false;
    }
    
    return true;
}

/* use port I/O transfer to read a certain amount of sectors from a drive */
bool ide_port_read_data(ide_bus_t *bus, uint64_t address, uint16_t sectors, void *data) {
    if (!bus || !data) {
        return false;
    }

    //get selected drive
    ide_device_t *selected = bus->selected == master ? &bus->master : &bus->slave;

    //packet interface not supported yet
    if (selected->ide_type != ide_pata && selected->ide_type != ide_sata) {
        return false;
    }

    //set transaction data (address and number of sectors)
    if (!ide_init_transaction(bus, address, sectors)) {
        return false;
    }

    if (selected->addr_mode == lba28 || selected->addr_mode == chs) {
        ide_command(bus, IDE_COMM_READ_PIO);
    } else if (selected->addr_mode == lba48) {
        ide_command(bus, IDE_COMM_READ_PIO_EXT);
    } else {
        return false; //unknwon addressing mode
    }

    //iterate through the sectors and read each word
    for (uint32_t i = 0; i < sectors * 256; i++) {
        ide_poll(bus); //waits

        if (ide_check_error(bus)) {
            return false;
        }

        *((uint16_t *) data + i) = ide_read(bus->io, IDE_BAR_OFFSET_DATA_REG);
    }

    return true;
}

/* use port I/O transfer to write a certain amount of sectors to a drive */
bool ide_port_write_data(ide_bus_t *bus, uint64_t address, uint16_t sectors, void *data) {
    if (!bus || !data) {
        return false;
    }

    //set transaction data (address and number of sectors)
    if (!ide_init_transaction(bus, address, sectors)) {
        return false;
    }

    //get selected drive
    ide_device_t *selected = bus->selected == master ? &bus->master : &bus->slave;
    uint8_t flush_command;

    //packet interface not supported yet
    if (selected->ide_type != ide_pata && selected->ide_type != ide_sata) {
        return false;
    }

    if (selected->addr_mode == lba28 || selected->addr_mode == chs) {
        ide_command(bus, IDE_COMM_WRITE_PIO);
        flush_command = IDE_COMM_CACHE_FLUSH;
    } else if (selected->addr_mode == lba48) {
        ide_command(bus, IDE_COMM_WRITE_PIO_EXT);
        flush_command = IDE_COMM_CACHE_FLUSH_EXT;
    } else {
        return false; //unknown addressing mode
    }

    for (uint16_t i = 0; i < sectors * 256; i++) {
        ide_poll(bus); //waits
        ide_write(bus->io, IDE_BAR_OFFSET_DATA_REG, *((uint16_t *) data + i));
        ide_command(bus, flush_command);

        if (i == sectors - 1) {
            ide_wait(bus);
        }
    }

    return true;
}

void ide_select_drive(ide_bus_t *bus, ide_drive_select_t drive) {
    if (!bus) {
        return;
    }

    ide_write(bus->io, IDE_BAR_OFFSET_DRIVE_HEAD, drive);
    bus->selected = drive;
}

void ide_command(ide_bus_t *bus, uint8_t command) {
    if (!bus) {
        return;
    }

    ide_write(bus->io, IDE_BAR_OFFSET_COMMAND, command);
    ide_wait(bus); //waits 400ns
}

uint8_t ide_status(ide_bus_t *bus) {
    return (uint8_t) ide_read(bus->io, IDE_BAR_OFFSET_STATUS);
}

/* resets disks on a bus (can't be reset individually) */
void ide_reset(ide_bus_t *bus) {
    uint8_t old = (uint8_t) ide_read(bus->control, 0);
    ide_write(bus->control, IDE_BAR_OFFSET_DEVICE_CONTROL, old | 4); //set bit 2
    ide_write(bus->control, IDE_BAR_OFFSET_DEVICE_CONTROL, old); //resets bit 2
}