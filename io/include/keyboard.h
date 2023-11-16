#pragma once
#include <include/types.h>
#define IN_BUFFER_SIZE 256
#define KEYCODE_NULL 0

/* keyboard control codes */

#define KEYCODE_LSHIFT_DOWN 1
#define KEYCODE_RSHIFT_DOWN 2
#define KEYCODE_LSHIFT_UP 3
#define KEYCODE_RSHIFT_UP 4
#define KEYCODE_LCTRL_DOWN 5
#define KEYCODE_RCTRL_DOWN 6
#define KEYCODE_LCTRL_UP 7
#define KEYCODE_RCTRL_UP 11
#define KEYCODE_ALT_DOWN 12
#define KEYCODE_ALT_UP 14
#define KEYCODE_ALTGR_DOWN 15
#define KEYCODE_ALTGR_UP 16
#define KEYCODE_CAPS_LOCK 21

/* terminal control codes */

#define KEYCODE_BACKSPACE 8
#define KEYCODE_TAB 9
#define KEYCODE_LINE_FEED 10
#define KEYCODE_CARRIAGE_RETURN 13
#define KEYCODE_UP_ARROW 17
#define KEYCODE_RIGHT_ARROW 18
#define KEYCODE_DOWN_ARROW 19
#define KEYCODE_LEFT_ARROW 20
#define KEYCODE_CANCEL 24
#define KEYCODE_ESCAPE 27
#define KEYCODE_DEL 127

#define IS_KEYBOARD_CONTROL(c) (c >= 1 && c <= 7 || c >= 11 && c <= 12 || c >= 14 && c <= 16 || c == 21)
#define IS_TERMINAL_CONTROL(c) (c >= 8 && c <= 10 || c == 13 || c >= 17 && c <= 20 || c == 24 || c == 27 || c == 127)
#define IS_PRINTABLE(c) (!IS_KEYBOARD_CONTROL(c) && !IS_TERMINAL_CONTROL(c) && c >= 0 && c < 128)

typedef enum {
    no_layout = 0,
    us_qwerty = 1,
    
} keyboard_layout_type_t;

typedef struct {
    keyboard_layout_type_t type;
    char map[95]; //map of characters
} keyboard_layout_t;


typedef enum {
    ps2,
    usb,
    bluetooth
} keyboard_source_t;

typedef struct {
    bool capslock;
    bool numlock;
    bool scroll_lock;
    bool shift;
    bool ctrl;
    bool altgr;
    bool alt;
    bool fn;
    keyboard_layout_t layout;
    keyboard_source_t source; //currently unused
} keyboard_status_t;

bool init_keyboard(void);
void keypressed(uint8_t code);
void keyboard_control(uint8_t control);
void keyboard_wait(char *msg);
void keyboard_wait_handler();