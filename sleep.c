#include <include/types.h>
#include <include/sleep.h>
#include <io/include/pit.h>
#include <int/include/int.h>

bool sleep_in_progress = false;
extern bool pit_ready; //defined in pit.c

//sleep ms milliseconds (uses PIT)
bool sleep(uint16_t ms) {
    if (!pit_ready) {
        return false;
    }
    
    if (ms == 0) {
        return true; //no need to bother the old pit
    }

    uint32_t count = ms;
    uint8_t hook_n;
    
    if ((hook_n = int_hook(0x17, sleep_notify)) == 0xFF) {
        return false;
    }

    pit_command(PIT_COMM_BIN | PIT_COMM_RATE_GENERATOR | PIT_COMM_CHANNEL0 | PIT_COMM_LOBYTE_HIBYTE);
    //divider 1190 (0x04A6)
    pit_write(0, 0xA6); //low byte
    pit_write(0, 0x04); //high byte
    pit_run();

    while(count-- > 0) {
        sleep_in_progress = true;
        while(sleep_in_progress);
    }

    pit_stop();
    return int_unhook(0x17, hook_n);
}

void sleep_notify() {
    sleep_in_progress = false;
}