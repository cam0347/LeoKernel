#pragma once
#include <include/types.h>
#include <io/include/pci.h>

/* I/O ports offset */

#define IDE_BAR_OFFSET_DATA_REG 0
#define IDE_BAR_OFFSET_ERROR_REG 1
#define IDE_BAR_OFFSET_FEATURES 1
#define IDE_BAR_OFFSET_SECTOR_COUNT 2
#define IDE_BAR_OFFSET_LBA_LO 3
#define IDE_BAR_OFFSET_LBA_MID 4
#define IDE_BAR_OFFSET_LBA_HI 5
#define IDE_BAR_OFFSET_DRIVE_HEAD 6
#define IDE_BAR_OFFSET_STATUS 7
#define IDE_BAR_OFFSET_COMMAND 7

/* command ports offset */

#define IDE_BAR_OFFSET_ALTERNATE_STATUS 0
#define IDE_BAR_OFFSET_DEVICE_CONTROL 0
#define IDE_BAR_OFFSET_DRIVE_ADDR 1

/* commands */

#define IDE_COMM_READ_PIO          0x20
#define IDE_COMM_READ_PIO_EXT      0x24
#define IDE_COMM_READ_DMA          0xC8
#define IDE_COMM_READ_DMA_EXT      0x25
#define IDE_COMM_WRITE_PIO         0x30
#define IDE_COMM_WRITE_PIO_EXT     0x34
#define IDE_COMM_WRITE_DMA         0xCA
#define IDE_COMM_WRITE_DMA_EXT     0x35
#define IDE_COMM_CACHE_FLUSH       0xE7
#define IDE_COMM_CACHE_FLUSH_EXT   0xEA
#define IDE_COMM_PACKET            0xA0
#define IDE_COMM_IDENTIFY_PACKET   0xA1
#define IDE_COMM_IDENTIFY          0xEC

typedef enum {
    master = 0xA0,
    slave = 0xB0
} ide_drive_select_t;

typedef enum {
    primary,
    secondary
} ide_bus_select_t;

typedef enum {
    chs,
    lba28,
    lba48
} ide_addressing_mode_t;

typedef enum {
    hard_disk,
    disk_reader
} ide_device_type_t;

typedef enum {
    ide_pata,
    ide_sata,
    ide_patapi,
    ide_satapi,
    ide_unknown
} ide_type_t;

typedef struct {
    ide_addressing_mode_t addr_mode;
    uint64_t sectors;
    bool exist;
    ide_device_type_t device_type;
    ide_type_t ide_type;
} ide_device_t;

typedef struct {
    uint32_t io;
    uint32_t control;
    ide_device_t master;
    ide_device_t slave;
    ide_drive_select_t selected;
} ide_bus_t;

typedef struct {
    pci_general_dev_t *pci_dev;
    ide_bus_t primary;     //bars 0 and 1
    ide_bus_t secondary;   //bars 2 and 3
    uint32_t bus_master;   //bar4
    bool dma_enabled;
} ide_controller_t;

typedef struct {
    uint32_t addr;        //physical address
    uint16_t size;        //byte count (size = 0 means 64k)
    uint16_t reserved;
} ide_prd_t;

bool ide_init(pci_general_dev_t *dev);
void ide_save_device_info(ide_device_t *dev, uint16_t *info);
bool ide_identify_drive(ide_bus_t *bus, ide_drive_select_t drive, void *info);
void ide_save_device_ide_type(ide_bus_t *bus, ide_drive_select_t select);
bool ide_set_channels_mode(pci_general_dev_t *dev);
void ide_wait(ide_bus_t *bus);
void ide_poll(ide_bus_t *bus);
bool ide_check_error(ide_bus_t *bus);
uint16_t ide_read(uint32_t bar, uint8_t reg);
void ide_write(uint32_t bar, uint8_t reg, uint16_t data);
bool ide_read_data(ide_bus_t *bus, uint64_t address, uint16_t sectors, void *data);
bool ide_write_data(ide_bus_t *bus, uint64_t address, uint16_t sectors, void *data);
void ide_select_drive(ide_bus_t *bus, ide_drive_select_t drive);
void ide_command(ide_bus_t *bus, uint8_t command);
uint8_t ide_status(ide_bus_t *bus);
void ide_reset(ide_bus_t *bus);
bool ide_init_transaction(ide_bus_t *bus, uint64_t address, uint16_t sectors);