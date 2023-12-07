#pragma once
#include <include/types.h>
#include <include/bootp.h>
#define TTY_COLOR_WHITE (tty_color_t) 0xFFFFFF
#define TTY_COLOR_BLACK (tty_color_t) 0x000000
#define TTY_COLOR_RED   (tty_color_t) 0xFF0000
#define TTY_COLOR_GREEN (tty_color_t) 0x00FF00
#define TTY_COLOR_BLUE  (tty_color_t) 0x0000FF
#define PRINTF_MAX_BUFFER 256

typedef uint32_t tty_color_t;

typedef struct {
    char c;
    tty_color_t background;
    tty_color_t foreground;
} tty_char_t;

bool init_tty(struct leokernel_boot_params);
tty_color_t get_tty_char_color();
void set_tty_char_fg(tty_color_t new_color);
void set_tty_char_bg(tty_color_t new_color);
tty_color_t get_tty_char_fg();
tty_color_t get_tty_char_bg();
void plot_pixel(uint64_t, uint64_t, tty_color_t);
void tty_clear();
bool tty_load_font(void *file, void **glyphs, uint8_t *glyph_height, uint8_t *glyph_width);
void putchar(char c, tty_color_t fg, tty_color_t bg);
void putchar_at(char c, uint32_t x, uint32_t y, tty_color_t fg, tty_color_t bg);
void tty_clear_cell(uint32_t x, uint32_t y);
void tty_backspace();
void printf(const char *fmt, ...);
void print_color(char *str, tty_color_t fg, tty_color_t bg);
void fail(char *);
void launch_splashscreen();
void tty_enable_bg();
void tty_disable_bg();
uint64_t get_tty_grid_height();
uint64_t get_tty_grid_width();