/*
IDE driver.
This file contains the functions to initialize the IDE devices found
in this system and save their data in a structured manner.
*/

#include <include/types.h>
#include <drv/ide/include/ide.h>
#include <drv/ide/include/ide_setup.h>
#include <io/include/pci.h>
#include <mm/include/obj_alloc.h>
#include <mm/include/kmalloc.h>
#include <include/mem.h>
#include <io/include/devs.h>

pool_t ide_devices_pool_id;
uint16_t ide_devices_pool_last_ind = 0;

/*
Given the PCI device, this function search and initialize every drive, and then it saves
them into the pool created by pci_init().
*/
bool ide_init(pci_general_dev_t *pci_dev) {
    if (!pci_dev || (pci_dev_type_t) pci_dev->header.class_code != mass_storage || (mass_storage_subclass_t) pci_dev->header.subclass != ide) {
        return false;
    }

    /* sets both buses to native mode */
    if (!ide_set_bus_mode(pci_dev, primary, compatibility) || !ide_set_bus_mode(pci_dev, secondary, compatibility)) {
        return false;
    }

    uint32_t bar0 = pci_dev->bar0 & 0xFFFFFFFC;
    uint32_t bar1 = pci_dev->bar1 & 0xFFFFFFFC;
    uint32_t bar2 = pci_dev->bar2 & 0xFFFFFFFC;
    uint32_t bar3 = pci_dev->bar3 & 0xFFFFFFFC;
    uint32_t bar4 = pci_dev->bar4 & 0xFFFFFFFC;

    bar0 = bar0 ? bar0 : 0x1F0; //primary, io
    bar1 = bar1 ? bar1 : 0x3F6; //primary, control
    bar2 = bar2 ? bar2 : 0x170; //secondary, io
    bar3 = bar3 ? bar3 : 0x376; //secondary, control

    //allocate the bus objects
    ide_bus_t *primary_bus, *secondary_bus;

    if  (!(primary_bus = (ide_bus_t *) kmalloc(sizeof(ide_bus_t))) || !(secondary_bus = (ide_bus_t *) kmalloc(sizeof(ide_bus_t)))) {
        kfree(primary_bus);
        kfree(secondary_bus);
        return false;
    }

    bool dma_enabled = (bool)(pci_dev->header.prog_if >> 7 & 1);
    ide_prd_t *primary_prdt = null, *secondary_prdt = null;

    if (dma_enabled) {
        if  (!(primary_prdt = (ide_prd_t *) kmalloc(sizeof(ide_prd_t))) || !(secondary_prdt = (ide_prd_t *) kmalloc(sizeof(ide_prd_t)))) {
            kfree(primary_prdt);
            kfree(secondary_prdt);
            dma_enabled = false; //if memory for the prdts can't be allocated, disable dma
        }
    }

    /* initialize bus objects */
    ide_bus_init(primary_bus, bar0, bar1, dma_enabled, primary_prdt);
    ide_bus_init(secondary_bus, bar2, bar3, dma_enabled, secondary_prdt);

    uint8_t primary_drives = ide_bus_test_and_add_drives(primary_bus);
    uint8_t secondary_drives = ide_bus_test_and_add_drives(secondary_bus);

    if (primary_drives == 0) {
        kfree(primary_prdt);
        kfree(primary_bus);
    }

    if (secondary_drives == 0) {
        kfree(secondary_prdt);
        kfree(secondary_bus);
    }

    return true;
}

/* 
Given a bus, test if master and slave drives are present.
If so, save their information retrieved by issuing the IDENTIFY command.
*/
uint8_t ide_bus_test_and_add_drives(ide_bus_t *bus) {
    if (!bus) {
        return 0;
    }

    uint8_t found = 0; //number of found drives [0-2]
    uint16_t device_info[256]; //buffer to save IDENTIFY command returned data

    ide_device_t master_drv, slave_drv;
    master_drv.bus = bus;
    master_drv.drive = master;
    slave_drv.bus = bus;
    slave_drv.drive = slave;

    //master device
    memclear(device_info, sizeof(device_info));
    if (ide_identify(&master_drv, device_info)) {
        ide_save_device_info(&master_drv, device_info);
        ide_save_device_ide_type(&master_drv);
        
        if (obj_pool_put(ide_devices_pool_id, (void *) &master_drv, ide_devices_pool_last_ind)) {
            ide_devices_pool_last_ind++;
            found++;
        }
    }

    //slave device
    memclear(device_info, sizeof(device_info));
    if (ide_identify(&slave_drv, device_info)) {
        ide_device_t dev;
        ide_save_device_info(&dev, device_info);
        ide_save_device_ide_type(&slave_drv);
        
        if (obj_pool_put(ide_devices_pool_id, (void *) &slave_drv, ide_devices_pool_last_ind)) {
            ide_devices_pool_last_ind++;
            found++;
        }
    }

    return found;
}

/*
Sends an IDENTIFY command to the drive and return it's informations.
Returns true if the drive is present, false otherwise.
*/
bool ide_identify(ide_device_t *dev, void *info) {
    if (!dev || !info || !dev->bus) {
        return false;
    }

    ide_bus_t *bus = dev->bus; //get the pointer to this drive's bus
    ide_select_drive(bus, dev->drive);
    ide_write_reg(bus->io_bar, IDE_BAR_OFFSET_SECTOR_COUNT, 0);
    ide_write_reg(bus->io_bar, IDE_BAR_OFFSET_LBA_LO, 0);
    ide_write_reg(bus->io_bar, IDE_BAR_OFFSET_LBA_MID, 0);
    ide_write_reg(bus->io_bar, IDE_BAR_OFFSET_LBA_HI, 0);
    ide_command(bus, IDE_COMM_IDENTIFY);

    if (ide_status(bus) == 0) {
        return false; //this device doesn't exist
    }

    if (ide_check_error(bus)) {
        return false;
    }

    while(ide_status(bus) >> 7 & 1); //waits for the busy bit (7th) clears
    while(!(ide_status(bus) >> 3 & 1) && !(ide_status(bus) & 1)); //waits for DRQ or ERR bit to set

    if (ide_check_error(bus)) {
        return false;
    }

    /* reads drive information (256 words = 512 bytes) */
    for (uint16_t i = 0; i < 256; i++) {
        *((uint16_t *) info + i) = ide_read_reg(bus->io_bar, IDE_BAR_OFFSET_DATA_REG);
    }

    return true;
}

/* save in the ide_device_t struct the informations returned by the ide_identify_device */
void ide_save_device_info(ide_device_t *dev, uint16_t *info) {
    if (!dev) {
        return;
    }

    dev->addr_mode = chs; //default addressing mode
    dev->magic = IDE_DEVICE_MAGIC;

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
}

/* resets the bus and save the device ide type */
void ide_save_device_ide_type(ide_device_t *dev) {
    if (!dev || !dev->bus) {
        return;
    }

    ide_bus_t *bus = dev->bus;
    ide_reset(bus);
    ide_wait(bus);
    ide_select_drive(bus, dev->drive);
    uint8_t cl = ide_read_reg(bus->io_bar, IDE_BAR_OFFSET_LBA_MID);
    uint8_t ch = ide_read_reg(bus->io_bar, IDE_BAR_OFFSET_LBA_HI);

    if (cl == 0x14 && ch == 0xEB) {
        dev->ide_type = ide_patapi;
    } else if (cl == 0x69 && ch == 0x96) {
        dev->ide_type = ide_satapi;
    } else if (cl == 0x00 && ch == 0x00) {
        dev->ide_type = ide_pata;
    } else if (cl == 0x3C && ch == 0xC3) {
        dev->ide_type = ide_sata;
    } else {
        dev->ide_type = ide_unknown;
    }
}

/* sets a bus to native or compatibility mode */
bool ide_set_bus_mode(pci_general_dev_t *pci_dev, ide_bus_select_t bus, ide_channel_mode_t mode) {
    if (!pci_dev) {
        return false;
    }

    uint8_t *prog_if = &pci_dev->header.prog_if;

    if (bus == primary && (*prog_if >> 1 & 1)) {
        if (mode == native) {
            *prog_if |= 1;
        } else {
            *prog_if &= 0xFE;
        }
    } else if (bus == secondary && (*prog_if >> 3 & 1)) {
        if (mode == native) {
            *prog_if |= (1 << 2);
        } else {
            *prog_if &= 0xFB;
        }
    } else {
        return false;
    }

    return true;
}

void ide_bus_init(ide_bus_t *bus, uint32_t io, uint32_t ctrl, bool dma, ide_prd_t *prdt) {
    if (!bus) {
        return;
    }

    bus->io_bar = io;
    bus->ctrl_bar = ctrl;
    bus->dma_enabled = dma;
    bus->prdt = prdt;
}