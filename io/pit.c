#include <include/types.h>
#include <io/include/pit.h>
#include <include/bcd.h>
#include <io/include/port_io.h>
#include <int/include/apic.h>

bool pit_ready = false;

void init_pit() {
    //set pit redirection entry, initially masked
    if (!apic_irq(PIT_IRQ, 0x17, fixed, physical, active_high, edge, true, 0)) {
        return; //che fare?
    }

    pit_ready = true;
}

/*
Sends a command to the PIT's port.
The command is structured like:
bit 0: binary 0 / bcd 1
bits 1-3: operating mode
bits 4-5: access mode (lobyte/hibyte only (3))
bits 6-7: select channel
*/
void pit_command(uint8_t command) {
    if (!pit_ready) {
        return;
    }

    outb(PIT_MODE_COMMAND_REGISTER_PORT, command);
}

/*
Writes data to a PIT channel.
The 'channel' parameters accept values from 0 to 2, other values will be ignored.
This function has to be called according to the access mode for this channel specified with pit_command().
*/
void pit_write(uint8_t channel, uint8_t data) {
    if (!pit_ready) {
        return;
    }

    uint8_t port;

    switch(channel) {
        case 0:
            port = PIT_CHANNEL0_DATA_PORT;
            break;

        case 1:
            port = PIT_CHANNEL1_DATA_PORT;
            break;

        case 2:
            port = PIT_CHANNEL2_DATA_PORT;
            break;

        default:
            return;
    }

    outb(port, data);
}

//unmasks pit irq
void pit_run() {
    if (!pit_ready) {
        return;
    }

    apic_set_mask(PIT_IRQ, false);
}

//masks pit irq
void pit_stop() {
    if (!pit_ready) {
        return;
    }

    apic_set_mask(PIT_IRQ, true);
}