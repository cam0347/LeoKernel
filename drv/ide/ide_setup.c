/*
(P)ATA controller driver.
This driver uses port I/O and the DMA transfer is currently being developed.
This driver uses status polling (no interrupts).
*/

#include <include/types.h>
#include <drv/ide/include/ide_setup.h>
#include <mm/include/obj_alloc.h>
#include <io/include/pci.h>
#include <drv/ide/include/ide.h>
#include <drv/ide/include/ide_io.h>
#include <io/include/devs.h>
#include <mm/include/kmalloc.h>
#include <mm/include/memory_manager.h>
#include <io/include/port_io.h>
#include <include/mem.h>

pool_t ide_controllers_pool_id;
uint8_t ide_controllers_pool_last_ind = 0;
uint8_t ide_drives_last_ind = 0;

/* initializes an ide controller and adds it to the pool */
bool ide_init(pci_general_dev_t *dev) {
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
            .selected = master,
            .ctrl = &ctrl
        },

        .secondary = {
            .io = bar2,
            .control = bar3,
            .selected = master,
            .ctrl = &ctrl
        },

        .dma_enabled = (bool)(dev->header.prog_if >> 7 & 1)
    };

    if (ctrl.dma_enabled) {
        void *prdt_primary, *prdt_secondary;

        //if the prdts memory can't be allocated, disable dma on this controller
        if ((prdt_primary = kalloc_page(IDE_PRDT_PAGES)) && (prdt_secondary = kalloc_page(IDE_PRDT_PAGES))) {
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

/* save in the ide_device_t struct the informations returned by the ide_identify_drive */
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