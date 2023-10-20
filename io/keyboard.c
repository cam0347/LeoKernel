/*
This module controls the keyboard device, whatever the source is.
The keyboard can be a PS/2 keyboard or a usb one.
IRQ1 is connected to the PS/2 keyboard (which in turn can be a real keyboard or an emulated one for backwards compatibility), usb are mapped 
*/

#include <include/types.h>
#include <tty/include/tty.h>
#include <io/include/keyboard.h>
#include <io/include/ps2_keyboard.h>
#include <int/include/apic.h>
#include <include/mem.h>
#include <io/include/files.h>

keyboard_status_t keyboard_status;
extern char *us_qwerty_map;
bool keyboard_controller_ready = false;

/*
this buffer will contain the typed characters.
when a key is pressed on a keyboard, if the character is visible it will be stored here.
*/
char in_buffer[IN_BUFFER_SIZE];
uint32_t in_buffer_cursor;
uint32_t buffer_stream_no; //descriptor of the file/stream to flush the buffer on

/*
this variable holds the ascii character that if entered will trigger
the in buffer to flush to the underlying stream
*/
char flush_trigger = 0;

void init_keyboard_controller() {
    //init keyboard status
    keyboard_status.capslock = false;
    keyboard_status.numlock = false;
    keyboard_status.scroll_lock = false;
    keyboard_status.shift = false;
    keyboard_status.ctrl = false;
    keyboard_status.altgr = false;
    keyboard_status.alt = false;
    keyboard_status.fn = false;
    keyboard_status.key_map = null;
    keyboard_status.layout = null;
    keyboard_status.source = ps2; //by default the keyboard input is set to be ps/2, when and if a usb keyboard is found it will be changed
    apic_irq(1, 0x18, fixed, physical, active_high, edge, false, 0); //adds ioapic redirection entry for irq1 to isr 17 (keyboard controller)
    memclear(in_buffer, IN_BUFFER_SIZE);
    in_buffer_cursor = 0;
    flush_trigger = KEYCODE_LINE_FEED;
    buffer_stream_no = STDIN_FILENO; //by default the stream is attached to the stdin
    keyboard_controller_ready = true;
}

bool set_keyboard_layout(keyboard_layout_t layout_code) {
    keyboard_status.layout = layout_code;

    switch(layout_code) {
        case us_qwerty:
            keyboard_status.key_map = us_qwerty_map;
            break;

        default:
            keyboard_status.layout = no_layout;
            return false;
    }

    return true;
}

/*
do something with the mixed ascii/custom key code.
this could be writing to the screen or controlling things.
*/
void keypressed(uint8_t keycode) {
    if (keycode == flush_trigger) {
        flush_in_buffer();
        in_buffer_cursor = 0;
        
        if (keycode != KEYCODE_LINE_FEED) {
            return;
        }
    }

    switch(keycode) {
        case KEYCODE_BACKSPACE:
            in_backspace();
            tty_backspace(); //to remove
            break;

        case KEYCODE_LINE_FEED:
            printf("\n"); //to remove
            break;

        case KEYCODE_CANCEL:
            //cancels text in the other direction
            break;

        case KEYCODE_ESCAPE:
            //???
            break;

        case KEYCODE_LSHIFT_DOWN:
        case KEYCODE_RSHIFT_DOWN:
            keyboard_status.shift = true;
            break;

        case KEYCODE_LSHIFT_UP:
        case KEYCODE_RSHIFT_UP:
            keyboard_status.shift = false;
            break;

        case KEYCODE_CAPS_LOCK:
            keyboard_status.capslock = !keyboard_status.capslock;
            break;

        case KEYCODE_LCTRL_DOWN:
        case KEYCODE_RCTRL_DOWN:
            keyboard_status.ctrl = true;
            break;

        case KEYCODE_LCTRL_UP:
        case KEYCODE_RCTRL_UP:
            keyboard_status.ctrl = false;
            break;

        case KEYCODE_ALTGR_DOWN:
            keyboard_status.altgr = true;
            break;

        case KEYCODE_ALTGR_UP:
            keyboard_status.altgr = false;
            break;

        default: {
            if (IS_ASCII(keycode)) {
                in_append((char) keycode);
                printf("%c", (char) keycode); //to remove
            }
        }
    }
}

/*
this function is called by the keyboard irq handler.
this function reads the scancode from the currently selected keyboard device and calls keypressed().
*/
void keyboard_read() {
    if (!keyboard_controller_ready) {
        return;
    }

    uint8_t keycode = 0;

    switch(keyboard_status.source) {
        case ps2:
            keycode = ps2_in(keyboard_status);
            break;

        case usb:
            keycode = 0; //get input with usb keyboard driver
            break;
        
        default:
            return;
    }

    if (keycode) {
        keypressed(keycode); //do something with the keycode
    }
}

/*---------------- in buffer functions ----------------*/

/*
writes the data to standard input and empties this buffer.
called when the user press a specific key.
*/
void flush_in_buffer() {
    write(buffer_stream_no, (void *) in_buffer, in_buffer_cursor); //writes to file descriptor 0 (stdin)
    memclear((void *) in_buffer, in_buffer_cursor);
}

__attribute__((always_inline))
inline void in_append(char c) {
    //if the buffer is full, flush it
    if (in_buffer_cursor >= IN_BUFFER_SIZE) {
        flush_in_buffer();
        in_buffer_cursor = 0;
    }

    in_buffer[in_buffer_cursor++] = c;
}

__attribute__((always_inline))
inline void in_backspace() {
    if (in_buffer_cursor > 0) {
        in_buffer[in_buffer_cursor--] = 0;
    }
}

/*
this function changes the input buffer underlying stream.
the buffer will be flushed onto that stream.
!warning: there's no control over the parameter, make sure it's a valid file descriptor before calling this function
*/
void in_buffer_set_stream(uint32_t stream) {
    buffer_stream_no = stream;
}