#include <drv/ide/include/ide_dma.h>
#include <drv/ide/include/ide_io.h>
#include <include/types.h>
#include <mm/include/memory_manager.h>
#include <io/include/port_io.h>

/* adds a prdt entry to the prdt of the specified bus */
bool ide_dma_prd_add(ide_controller_t *ctrl, ide_bus_select_t bus, ide_prd_t prd) {
    if (!ctrl) {
        return false;
    }

    //get the pointer to the right prdt
    ide_prd_t *prdt = (ide_prd_t *)(((uint64_t) inl(ctrl->bus_master + (bus == master ? 4 : 0x0C))) << 32);
    uint16_t prd_n = IDE_PRDT_PAGES * PAGE_SIZE / sizeof(ide_prd_t);
    
    for (uint16_t i = 0; i < prd_n; i++) {
        ide_prd_t *entry = prdt + i;

        //if this entry is not present, store the prd here
        if (!entry->addr) {
            *entry = prd;
            return true;
        }
    }

    return false;
}

/* use DMA transfer mode to read a certain amount of sectors to a drive */
bool ide_dma_read_data(ide_bus_t *bus, uint64_t address, uint16_t sectors, void *data) {
    if (!bus || !data) {
        return false;
    }

    //set transaction data
    if (!ide_init_transaction(bus, address, sectors)) {
        return false;
    }


}