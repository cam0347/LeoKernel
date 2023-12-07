#pragma once
#include <include/types.h>
#include <io/include/pci.h>
#include <mm/include/obj_alloc.h>

typedef enum {
    scsi,
    ide,
    floppy_disk,
    ipi,
    raid,
    ata,
    sata,
    serial_scsi,
    nvm,
    other = 0x80
} mass_storage_subclass_t;

typedef struct {
    uint8_t type: 1;

    union {
        struct {
            uint8_t type: 2;
            uint8_t prefetchable: 1;
            uint32_t addr: 28;
        } ms;

        struct {
            uint8_t res: 1;
            uint32_t addr: 30; 
        } io;
    } data;
} pci_bar_t;

//---------------------------------------------------------------------------------
typedef struct {
    mass_storage_subclass_t type;
    pool_t pool_id;
} ms_dev_t;

bool devs_config_mass_storage(pci_general_dev_t *dev);