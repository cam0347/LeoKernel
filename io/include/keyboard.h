#pragma once
#include <include/types.h>
#define IN_BUFFER_SIZE 256
#define KEYCODE_NULL 0
#define KEYCODE_LSHIFT_DOWN 1
#define KEYCODE_RSHIFT_DOWN 2
#define KEYCODE_LSHIFT_UP 3
#define KEYCODE_RSHIFT_UP 4
#define KEYCODE_LCTRL_DOWN 5
#define KEYCODE_RCTRL_DOWN 6
#define KEYCODE_LCTRL_UP 7
#define KEYCODE_BACKSPACE 8
#define KEYCODE_TAB 9
#define KEYCODE_LINE_FEED 10
#define KEYCODE_RCTRL_UP 11
#define KEYCODE_ALT_DOWN 12
#define KEYCODE_CARRIAGE_RETURN 13
#define KEYCODE_ALT_UP 14
#define KEYCODE_ALTGR_DOWN 15
#define KEYCODE_ALTGR_UP 16
#define KEYCODE_UP_ARROW 17
#define KEYCODE_RIGHT_ARROW 18
#define KEYCODE_DOWN_ARROW 19
#define KEYCODE_LEFT_ARROW 20
#define KEYCODE_CAPS_LOCK 21
#define KEYCODE_CANCEL 24
#define KEYCODE_ESCAPE 27
#define KEYCODE_DEL 127
#define IS_ASCII(c) (c >= 32 && c <= 126)

typedef enum {
    no_layout = 0,
    us_qwerty = 1
} keyboard_layout_t;

/*
typedef struct {
    char name[9]; //name of this layout, format: ##@@@@@@ (## nation code, @@@@@@ qwerty or qwertz)
    char map[95]; //map of characters
} keyboar_layout_t;
*/

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
    char *key_map; //will hold a pointer to the current keyboard key map
    keyboard_layout_t layout;
    keyboard_source_t source;
} keyboard_status_t;

void init_keyboard_controller();
bool set_keyboard_layout(keyboard_layout_t layout_code);
void keypressed(uint8_t scancode);
void keyboard_read();
void flush_in_buffer();
void in_append(char c);
void in_backspace();
void in_buffer_set_stream(uint32_t stream);