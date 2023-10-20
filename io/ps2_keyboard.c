#include <include/types.h>
#include <io/include/ps2_keyboard.h>
#include <io/include/keyboard.h>
#include <io/include/port_io.h>

bool capslock = false;
bool numlock = false;
bool scrolllock = false;
bool ps2_keyboard_enabled = true;

/*
this map contains the 'pressed' codes
*/
char ps2_keymap[] = {
    KEYCODE_ESCAPE, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', KEYCODE_BACKSPACE, KEYCODE_TAB, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o',
    'p', '[', ']', KEYCODE_LINE_FEED, KEYCODE_LCTRL_DOWN, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '\'', KEYCODE_LSHIFT_DOWN, '\\', 'z', 'x', 'c',
    'v', 'b', 'n', 'm', ',', '.', '/', KEYCODE_RSHIFT_DOWN, '*', KEYCODE_ALT_DOWN, ' ', KEYCODE_CAPS_LOCK, '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3',
    '0', '.', KEYCODE_LCTRL_UP, KEYCODE_LSHIFT_UP, KEYCODE_RSHIFT_UP, KEYCODE_ALT_UP
};

/*
reads the scan code of the key pressed
*/
uint8_t ps2_in(keyboard_status_t keyboard_status) {
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

    //make capital letters if shift or caps lock is pressed
    if (ret >= 'a' && ret <= 'z' && (keyboard_status.shift || keyboard_status.capslock) && !prev_ext) {
        ret -= 32;
    }/* else if (keyboard_status.altgr) {
        ret = ps2_switch_altgr(ret);
    }*/

    return ret;
}

uint8_t ps2_switch_scancode(uint8_t s) {
    if (s >= 1 && s <= 0x3A) {
        return ps2_keymap[s - 1];
    } else if (s >= 0x47 && s <= 0x53) {
        return ps2_keymap[s - 0x47];
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

uint8_t ps2_switch_altgr(uint8_t scancode) {
    switch(scancode) {
        case 0x27:
            return '@';
            break;

        case 0x28:
            return '#';
            break;

        default:
            return 0;
    }
}