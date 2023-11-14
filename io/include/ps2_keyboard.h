#pragma once
#include <include/types.h>
#include <io/include/keyboard.h>
#define PS2_EXT_SCANCODE 0xE0

uint8_t ps2_in(void);
uint8_t ps2_switch_scancode(uint8_t s);
uint8_t ps2_switch_ext_scancode(uint8_t scancode);
uint8_t ps2_switch_altgr(uint8_t scancode);