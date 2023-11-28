#pragma once
#include <include/types.h>
#include <drv/ide/include/ide.h>

typedef struct {
    uint32_t addr;        //physical address to transfer the data to/from
    uint16_t size;        //byte count (size = 0 means 64k), must match the sectors number or the controller will freak out
    uint16_t reserved;    //reserved (should be zero) except the last bit (15th), if set it means this is the last prdt entry
} ide_prd_t;

typedef struct {
    ide_prd_t *prd;
    ide_transaction_type_t type;
    uint64_t lba;
} ide_dma_transaction_t;