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

#define IDE_PRDT_PAGES 1

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

typedef enum {
    ide_trns_read,
    ide_trns_write
} ide_transaction_type_t;

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
    struct ide_controller *ctrl;
} ide_bus_t;

typedef struct ide_controller {
    pci_general_dev_t *pci_dev;
    ide_bus_t primary;     //bars 0 and 1
    ide_bus_t secondary;   //bars 2 and 3
    uint32_t bus_master;   //bar4
    bool dma_enabled;
} ide_controller_t;