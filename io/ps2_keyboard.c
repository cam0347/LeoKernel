#include <include/types.h>
#include <io/include/ps2_keyboard.h>
#include <io/include/keyboard.h>
#include <io/include/port_io.h>

extern const char keyboard_codes[]; //defined in keyboard.c

/*
reads the scan code of the key pressed
*/
uint8_t ps2_in(void) {
    if (!(inb(0x64) & 1)) {
        return 0;
    }

    uint8_t scancode = inb(0x60);
    char ret = 0;
    static bool prev_ext = false;

    if (scancode == PS2_EXT_SCANCODE) {
        prev_ext = true;
        return 0;
    }

    if (prev_ext) {
        ret = ps2_switch_ext_scancode(scancode);
        prev_ext = false;
    } else {
        ret = ps2_switch_scancode(scancode);
    }

    return ret;
}

uint8_t ps2_switch_scancode(uint8_t s) {
    if (s >= 1 && s <= 0x3A) {
        return keyboard_codes[s - 1];
    } else if (s >= 0x47 && s <= 0x53) {
        return keyboard_codes[s - 0x47];
    }
    
    switch(s) {
        case 0x9D:
            return KEYCODE_LCTRL_UP;
            break;

        case 0xAA:
            return KEYCODE_LSHIFT_UP;
            break;

        case 0xB6:
            return KEYCODE_RSHIFT_UP;
            break;

        case 0xB8:
            return KEYCODE_ALT_UP;
            break;
    }
    
    return 0;
}

uint8_t ps2_switch_ext_scancode(uint8_t scancode) {
    switch(scancode) {
        case 0x38:
            return KEYCODE_ALTGR_DOWN;
            break;

        case 0xB8:
            return KEYCODE_ALTGR_UP;
            break;

        case 0x1D:
            return KEYCODE_RCTRL_DOWN;
            break;

        case 0x9D:
            return KEYCODE_RCTRL_UP;
            break;

        default:
            return 0;
            break;
    }
}