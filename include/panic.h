#pragma once
#include <include/types.h>
#include <tty/include/tty.h>
#define PANIC_SCREEN_BACKGROUND (tty_color_t) 227 << 16 | 38 << 8 | 54

void panic(char *reason);