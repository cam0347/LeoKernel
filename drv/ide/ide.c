/*
(P)ATA (also referred to IDE) is a standard interface for mass storage devices.
PATA is the original one, parallel communcation, SATA uses serial communication.
An IDE cable can link up to 2 drives directly to the motherboard, those two drives
are called master and slave (although they're at the same level).
On a motherboard there are two slots, the primary and the secondary, which can support two
drives each.
The default communication method with ATA devices is through port I/O (ATA PIO).
This driver handles (P)ATA PIO mode and packet interface (PI) and their IDE DMA.
IDE drives can run in native or compatibility mode, this driver supports only native mode.

Bars of a ide controller:
primary channel:
bar0: pci native mode (8)
bar1: control port (4)

secondary channel:
bar2: native mode (8)
bar3: control port (4)

bar4: bus master (16)

Addressing modes: LBA28, LBA48, CHS
Reading modes: PIO, single word DMA, double word DMA, ultra DMA
Polling modes: IRQ, polling status

** THIS DRIVER USES STATUS POLLING, DMA IS CURRENTLY UNSUPPORTED **
*/

#include <io/include/pci.h>
#include <drv/ide/include/ide.h>
#include <mm/include/obj_alloc.h>
#include <io/include/port_io.h>
#include <io/include/devs.h>
#include <mm/include/kmalloc.h>
#include <include/mem.h>
#include <mm/include/memory_manager.h>

pool_t ide_controllers_pool_id;
uint8_t ide_controllers_pool_last_ind = 0;
uint8_t ide_drives_last_ind = 0;

/* checks and initialize master and slave drives (if any) for each bus of an ide controller */
bool ide_init(pci_general_dev_t *dev) {
    //checks if this is an actual IDE controller (class code 1, subclass 1)
    if (!dev || (pci_dev_type_t) dev->header.class_code != mass_storage || (mass_storage_subclass_t) dev->header.subclass != ide) {
        return false;
    }

    //set channels to native mode or return if unsupported
    if (!ide_set_channels_mode(dev)) {
        return false; //one of the channels doesn't support native mode
    }

    uint32_t bar0 = dev->bar0 & 0xFFFFFFFC;
    uint32_t bar1 = dev->bar1 & 0xFFFFFFFC;
    uint32_t bar2 = dev->bar2 & 0xFFFFFFFC;
    uint32_t bar3 = dev->bar3 & 0xFFFFFFFC;
    uint32_t bar4 = dev->bar4 & 0xFFFFFFFC;

    bar0 = bar0 ? bar0 : 0x1F0; //primary bus io
    bar1 = bar1 ? bar1 : 0x3F6; //primary bus control
    bar2 = bar2 ? bar2 : 0x170; //secondary bus io
    bar3 = bar3 ? bar3 : 0x376; //secondary bus control

    /*
    save some initial informations about this controller.
    further informations will be saved later.
    by default, the selected drive is the master, even if it doesn't exist,
    this information will be fixed later
    */
    ide_controller_t ctrl = {
        .pci_dev = dev,
        .primary = {
            .io = bar0,
            .control = bar1,
            .selected = master
        },

        .secondary = {
            .io = bar2,
            .control = bar3,
            .selected = master
        },

        .dma_enabled = (bool)(dev->header.prog_if >> 7 & 1)
    };

    if (ctrl.dma_enabled) {
        void *prdt_primary, *prdt_secondary;

        //if the prdts memory can't be allocated, disable dma on this controller
        if ((prdt_primary = kalloc_page(1)) && (prdt_secondary = kalloc_page(1))) {
            ctrl.bus_master = bar4;
            outl(ctrl.bus_master + 4, (uint64_t) prdt_primary >> 32 & 0xFFFFFFFF); //write primary bus prdt address
            outl(ctrl.bus_master + 0x0C, (uint64_t) prdt_secondary >> 32 & 0xFFFFFFFF); //write secondary bus prdt address
        } else {
            kfree_page(prdt_primary);
            kfree_page(prdt_secondary);
            ctrl.bus_master = 0;
            ctrl.dma_enabled = false;
        }
    }

    //test drives individually
    void *disk_info;

    if (!(disk_info = kmalloc(512))) {
        return false;
    }

    //primary bus, master drive
    memclear(disk_info, 512);
    if (ide_identify_drive(&ctrl.primary, master, disk_info)) {
        ide_save_device_info(&ctrl.primary.master, disk_info);
        ide_save_device_ide_type(&ctrl.primary, master);
    }

    //primary bus, slave drive
    memclear(disk_info, 512);
    if (ide_identify_drive(&ctrl.primary, slave, disk_info)) {
        ide_save_device_info(&ctrl.primary.slave, disk_info);
        ide_save_device_ide_type(&ctrl.primary, slave);
    }

    //secondary bus, master drive
    memclear(disk_info, 512);
    if (ide_identify_drive(&ctrl.secondary, master, disk_info)) {
        ide_save_device_info(&ctrl.secondary.master, disk_info);
        ide_save_device_ide_type(&ctrl.secondary, master);
    }

    //secondary bus, slave drive
    memclear(disk_info, 512);
    if (ide_identify_drive(&ctrl.secondary, slave, disk_info)) {
        ide_save_device_info(&ctrl.secondary.slave, disk_info);
        ide_save_device_ide_type(&ctrl.secondary, slave);
    }

    //save selected drives
    ctrl.primary.selected = (uint8_t) ide_read(ctrl.primary.io, IDE_BAR_OFFSET_DRIVE_HEAD) & 0xB0;
    ctrl.secondary.selected = (uint8_t) ide_read(ctrl.secondary.io, IDE_BAR_OFFSET_DRIVE_HEAD) & 0xB0;

    //save this controller's info into the ide disks pool (created during pci initialization)
    return obj_pool_put(ide_controllers_pool_id, (void *) &ctrl, ide_controllers_pool_last_ind++);
}

/* save in the ide_device_t struct the informations returned by the IDENTIFY command */
void ide_save_device_info(ide_device_t *dev, uint16_t *info) {
    if (!dev) {
        return;
    }

    dev->exist = true;
    dev->addr_mode = chs; //default addressing mode

    if (*(info + 83) >> 10 & 1) {
        dev->addr_mode = lba48;
    }

    uint32_t lba28_sectors = *(uint32_t *)(info + 60);
    uint64_t lba48_sectors = *(uint64_t *)(info + 100);

    if (lba28_sectors != 0) {
        dev->addr_mode = lba28;
        dev->sectors = lba28_sectors;
    }

    if (lba48_sectors != 0) {
        dev->addr_mode = lba48;
        dev->sectors = lba48_sectors;
    }

    dev->device_type = hard_disk; //u sure about this?
}

/*
Send IDENTIFY command and retrieve some informations.
This function is used to determine whether a specific drive exists or not.
To return the 512 bytes of data the info parameter is a pointer to a
buffer previously allocated.
*/
bool ide_identify_drive(ide_bus_t *bus, ide_drive_select_t drive, void *info) {
    if (!bus || !info) {
        return false;
    }

    ide_select_drive(bus, drive);
    ide_write(bus->io, IDE_BAR_OFFSET_SECTOR_COUNT, 0);
    ide_write(bus->io, IDE_BAR_OFFSET_LBA_LO, 0);
    ide_write(bus->io, IDE_BAR_OFFSET_LBA_MID, 0);
    ide_write(bus->io, IDE_BAR_OFFSET_LBA_HI, 0);
    ide_command(bus, IDE_COMM_IDENTIFY);

    /* if the returned status is 0 this drive doesn't exist */
    if (ide_status(bus) == 0) {
        return false;
    }

    while(ide_status(bus) >> 7 & 1); //waits for the busy bit (7th) clears

    /* this was commented because it returned false if the drive wasn't a pata drive */
    /* if LBAmid or LBAhi are not 0, this is not an ATA drive, return false */
    /*if (ide_read(bus->io, IDE_BAR_OFFSET_LBA_MID) != 0 || ide_read(bus->io, IDE_BAR_OFFSET_LBA_HI) != 0) {
        return false;
    }*/

    /* waits for DRQ or ERR bit to set */
    while(!(ide_status(bus) >> 3 & 1) && !(ide_status(bus) & 1));

    /* reads drive information */
    for (uint16_t i = 0; i < 256; i++) {
        *((uint16_t *) info + i) = ide_read(bus->io, IDE_BAR_OFFSET_DATA_REG);
    }

    return true;
}

/* resets the bus and save the device ide type */
void ide_save_device_ide_type(ide_bus_t *bus, ide_drive_select_t select) {
    ide_device_t *drive = select == master ? &bus->master : &bus->slave;

    ide_reset(bus);
    ide_wait(bus);
    ide_select_drive(bus, select);
    uint8_t cl = ide_read(bus->io, IDE_BAR_OFFSET_LBA_MID);
    uint8_t ch = ide_read(bus->io, IDE_BAR_OFFSET_LBA_HI);

    if (cl == 0x14 && ch == 0xEB) {
        drive->ide_type = ide_patapi;
    } else if (cl == 0x69 && ch == 0x96) {
        drive->ide_type = ide_satapi;
    } else if (cl == 0x00 && ch == 0x00) {
        drive->ide_type = ide_pata;
    } else if (cl == 0x3C && ch == 0xC3) {
        drive->ide_type = ide_sata;
    } else {
        drive->ide_type = ide_unknown;
    }
}

bool ide_set_channels_mode(pci_general_dev_t *dev) {
    if (!dev || (pci_dev_type_t) dev->header.class_code != mass_storage || (mass_storage_subclass_t) dev->header.subclass != ide) {
        return false;
    }

    uint8_t *prog_if = &dev->header.prog_if;

    bool channel1_native_mode = *prog_if & 1;
    bool channel2_native_mode = *prog_if >> 2 & 1;
    bool channel1_mode_changeable = *prog_if >> 1 & 1;
    bool channel2_mode_changeable = *prog_if >> 3 & 1;
    //bool supports_dma = *prog_if >> 7 & 1;

    if (!channel1_native_mode && channel1_mode_changeable) {
        *prog_if |= 1;
    } else if (!channel1_native_mode) {
        return false; //channel 1 unsupported mode
    }

    if (!channel2_native_mode && channel2_mode_changeable) {
        *prog_if |= (1 << 2);
    } else if (!channel2_native_mode) {
        return false; //channel 2 unsupported mode
    }

    return true;
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

bool ide_check_error(ide_bus_t *bus) {
    uint8_t status = ide_status(bus);
    return (status & 1) || (status >> 5 & 1);
}

uint16_t ide_read(uint32_t bar, uint8_t reg) {
    return inw(bar + reg);
}

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

/*
commands a drive to read a certain amount of sectors.
this function implements both standard and packet interface communication modes.
note that this function expects the right drive to be selected.
*/
bool ide_read_data(ide_bus_t *bus, uint64_t address, uint16_t sectors, void *data) {
    if (!bus || !data) {
        return false;
    }

    //set transaction data (address and number of sectors)
    if (!ide_init_transaction(bus, address, sectors)) {
        return false;
    }

    //get selected drive
    ide_device_t *selected = bus->selected == master ? &bus->master : &bus->slave;

    //packet interface not supported yet
    if (selected->ide_type != ide_pata && selected->ide_type != ide_sata) {
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

/*
commands a drive to write a certain amount of sectors.
this function implements both standard and packet interface communication modes.
note that this function expects the right drive to be selected.
*/
bool ide_write_data(ide_bus_t *bus, uint64_t address, uint16_t sectors, void *data) {
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