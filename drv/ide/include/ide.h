#pragma once
#include <include/types.h>

/* I/O bar offsets */

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

/* command bar offsets */

#define IDE_BAR_OFFSET_ALT_STATUS 0
#define IDE_BAR_OFFSET_DEV_CTRL 0
#define IDE_BAR_OFFSET_DRIVE_ADDR 1

/* commands */

#define IDE_COMM_READ_PIO        0x20
#define IDE_COMM_READ_PIO_EXT    0x24
#define IDE_COMM_READ_DMA        0xC8
#define IDE_COMM_READ_DMA_EXT    0x25
#define IDE_COMM_WRITE_PIO       0x30
#define IDE_COMM_WRITE_PIO_EXT   0x34
#define IDE_COMM_WRITE_DMA       0xCA
#define IDE_COMM_WRITE_DMA_EXT   0x35
#define IDE_COMM_CACHE_FLUSH     0xE7
#define IDE_COMM_CACHE_FLUSH_EXT 0xEA
#define IDE_COMM_PACKET          0xA0
#define IDE_COMM_IDENTIFY_PACKET 0xA1
#define IDE_COMM_IDENTIFY        0xEC

#define IDE_DEVICE_MAGIC 0xC0

typedef enum {
    ide_pata,
    ide_sata,
    ide_patapi,
    ide_satapi,
    ide_unknown
} ide_type_t;

typedef enum {
    chs,
    lba28,
    lba48
} ide_addr_mode_t;

typedef enum {
    master = 0xA0,
    slave = 0xB0
} ide_drive_select_t;

typedef enum {
    primary,
    secondary
} ide_bus_select_t;

typedef enum {
    native,
    compatibility
} ide_channel_mode_t;

/* defines an entry of a PRDT */
typedef struct {
    uint8_t foo;
} ide_prd_t;

/* defines an IDE bus */
typedef struct {
    uint32_t io_bar;             //PCI bar for I/O
    uint32_t ctrl_bar;           //PCI bar for control
    bool dma_enabled;            //true if this bus supports DMA
    ide_prd_t *prdt;             //pointer to this bus prdt
} ide_bus_t;

/* defines an IDE device */
typedef struct {
    ide_bus_t *bus;              //pointer to this drive's bus
    ide_type_t ide_type;         //type of ide device (pata, sata, patapi, satapi)
    ide_addr_mode_t addr_mode;   //addressing mode (chs, lba28, lba48)
    uint64_t sectors;            //number of sectors of this drive
    ide_drive_select_t drive;    //master or slave drive
    uint8_t magic;               //magic value used to retrieve this object
} ide_device_t;

uint16_t ide_read_reg(uint32_t bar, uint8_t reg);
void ide_write_reg(uint32_t bar, uint8_t reg, uint16_t data);
void ide_select_drive(ide_bus_t *bus, ide_drive_select_t drive);
bool ide_init_transaction(ide_device_t *dev, uint64_t address, uint64_t sectors);
uint8_t ide_status(ide_bus_t *bus);
void ide_wait(ide_bus_t *bus);
void ide_poll(ide_bus_t *bus);
uint8_t ide_error(ide_bus_t *bus);
bool ide_check_error(ide_bus_t *bus);
void ide_reset(ide_bus_t *bus);
void ide_command(ide_bus_t *bus, uint8_t command);