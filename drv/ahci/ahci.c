#include <include/types.h>
#include <drv/ahci/include/ahci.h>
#include <include/mem.h>

bool ahci_command(ahci_device_t *dev, ahci_comm_header_t comm) {
    if (!dev) {
        return false;
    }

    ahci_comm_header_t *slot = ahci_cl_get_slot(dev);

    if (!slot) {
        return false; //we run out of slots
    }

    memcpy(slot, &comm, sizeof(ahci_comm_header_t));
    return true;
}

ahci_comm_header_t *ahci_cl_get_slot(ahci_device_t *dev) {
    if (!dev || !dev->port) {
        return null;
    }

    uint64_t cl_lo = dev->port->clb_lo;
    uint64_t cl_hi = dev->port->clb_hi;
    ahci_comm_header_t *cl = (ahci_comm_header_t *)(cl_hi << 32 | cl_lo);

    if (!cl) {
        return false;
    }

    uint32_t slots = dev->ctrl->host_capability;

    for (uint8_t i = 0; i < slots; i++) {
        if (!(dev->port->command_issue >> i & 1)) {
            dev->port->command_issue |= (1 << i); //set this slot as used
            return cl + i; //return the pointer to this slot's command header
        }
    }

    return null;
}