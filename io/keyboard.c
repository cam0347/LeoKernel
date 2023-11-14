#include <io/include/keyboard.h>
#include <include/types.h>
#include <io/include/files.h>
#include <int/include/apic.h>
#include <tty/include/term.h>

keyboard_status_t ks;
bool keyboard_ready = false;

/*
this array is the internal representation of characters (printable and control), it's a mix of ASCII codes and custom codes.
keyboard drivers must convert their scancodes into these codes to be passed to keypressed().
*/
const char keyboard_codes[] = {
    KEYCODE_ESCAPE, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', KEYCODE_BACKSPACE, KEYCODE_TAB, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o',
    'p', '[', ']', KEYCODE_LINE_FEED, KEYCODE_LCTRL_DOWN, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '\'', KEYCODE_LSHIFT_DOWN, '\\', 'z', 'x', 'c',
    'v', 'b', 'n', 'm', ',', '.', '/', KEYCODE_RSHIFT_DOWN, '*', KEYCODE_ALT_DOWN, ' ', KEYCODE_CAPS_LOCK, '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3',
    '0', '.', KEYCODE_LCTRL_UP, KEYCODE_LSHIFT_UP, KEYCODE_RSHIFT_UP, KEYCODE_ALT_UP
};

/* initialize keyboard input subsystem */
bool init_keyboard(void) {
    ks.capslock = false;
    ks.numlock = false;
    ks.scroll_lock = false;
    ks.shift = false;
    ks.ctrl = false;
    ks.altgr = false;
    ks.alt = false;
    ks.fn = false;
    ks.source = ps2; //by default the keyboard input is set to be ps/2, when and if a usb keyboard is found it will be changed
    apic_irq(1, 0x18, fixed, physical, active_high, edge, false, 0); //ps2 irq
    keyboard_ready = true;
}

/*
this function is called by every keyboard driver.
if it's a printable character calls the appropriate function,
if it's a control character calls the function that handles the controlled device (keyboard or terminal).
*/
void keypressed(uint8_t code) {
    if (!keyboard_ready || code == 0) {
        return;
    }

    if (IS_PRINTABLE(code)) {
        term_putc(code);
        /*
        calls the terminal function to pass the character.
        that function has the responsibility to check whether the printable character
        is part of a keybinding (checks the keyboard status
        for active keybinding triggers (shift, ctrl, alt, altgr)),
        and to interpret the character being pressed (if a alt or capslock is active, altgr and so on)
        */
    } else if (IS_KEYBOARD_CONTROL(code)) {
        keyboard_control(code);
    } else if (IS_TERMINAL_CONTROL(code)) {
        term_control(code);
    }
}

/* edits keyboard status (keys interpretation) */
void keyboard_control(uint8_t control) {
    if (!IS_KEYBOARD_CONTROL(control)) {
        return;
    }

    switch(control) {
        case KEYCODE_LSHIFT_DOWN:
        case KEYCODE_RSHIFT_DOWN:
            ks.shift = true;
            break;

        case KEYCODE_LSHIFT_UP:
        case KEYCODE_RSHIFT_UP:
            ks.shift = false;
            break;

        case KEYCODE_LCTRL_DOWN:
        case KEYCODE_RCTRL_DOWN:
            ks.ctrl = true;
            break;

        case KEYCODE_LCTRL_UP:
        case KEYCODE_RCTRL_UP:
            ks.ctrl = false;
            break;

        case KEYCODE_ALT_DOWN:
            ks.alt = true;
            break;

        case KEYCODE_ALT_UP:
            ks.alt = false;
            break;

        case KEYCODE_ALTGR_DOWN:
            ks.altgr = true;
            break;

        case KEYCODE_ALTGR_UP:
            ks.altgr = false;
            break;

        case KEYCODE_CAPS_LOCK:
            ks.capslock = !ks.capslock;
            break;
    }
}