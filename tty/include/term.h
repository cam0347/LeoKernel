#pragma once
#include <include/types.h>
#define TERMINAL_COMMAND_LENGTH 128
#define CLEAR_SCREEN_ON_TERMINAL_START false

void init_terminal(void);
void term_putc(char c);
void term_control(uint8_t code);