#include <tty/include/tty.h>
#include <include/types.h>
#include <include/assert.h>
#include <include/mem.h>
#include <include/string.h>
#include <include/bootp.h>
#include <tty/include/psf_font.h>
#include <include/low_level.h>
#include <include/math.h>
#include <include/stdargs.h>
#include <tty/include/vsnprintf.h>
#include <tty/include/hue.h>
#include <tty/include/def_colors.h>
#include <mm/include/kmalloc.h>

void *fb, *glyphs;
uint64_t tty_height, tty_width, tty_size;

struct {
    uint64_t x;
    uint64_t y;
} tty_cursor;

struct {
    uint64_t height;
    uint64_t width;
    uint8_t cell_height;
    uint8_t cell_width;
    uint32_t cell_pitch;
} tty_grid;

/* this is the default background and foreground for text */
struct {
    tty_color_t foreground;
    tty_color_t background;
    bool bg_transparent;
} tty_color;

bool tty_ready = false;
bool tem_enabled = true; //text editor mode selector

//initialize terminal
bool init_tty(struct leokernel_boot_params bp) {
    if (tty_ready) {
        return false;
    }

    tty_load_font(bp.font, &glyphs, &tty_grid.cell_height, &tty_grid.cell_width);

    fb = bp.frame_buffer;

    tty_height = bp.video_height;
    tty_width = bp.video_width;
    tty_size = bp.frame_buffer_size;

    tty_color.foreground = GREY_COLOR;
    tty_color.background = TTY_COLOR_BLACK;
    tty_color.bg_transparent = true;

    tty_grid.cell_pitch = bp.video_pitch;
    tty_grid.width = tty_width / tty_grid.cell_width;
    tty_grid.height = tty_height / tty_grid.cell_height;

    tty_cursor.x = 0;
    tty_cursor.y = 0;

    tem_enabled = false;
    tty_tem_line_number();

    tty_clear();
    tty_ready = true;

    return true;
}

void tty_load_font(void *file, void **glyphs, uint8_t *glyph_height, uint8_t *glyph_width) {
    assert_true(file != null);
    bool psf1 = (*(uint16_t *) file == PSF1_HEADER_MAGIC);
    bool valid = psf1 || (*(uint32_t *) file == PSF2_HEADER_MAGIC);

    if (!valid) {
        sys_hlt(); //che si fa?
    }

    if (psf1) {
        psf1_header *header = (psf1_header *) file;
        uint8_t size = header->character_size; //number of bytes per glyph, also glyph height (the width is always 8 pixels)
        *glyph_height = size;
        *glyph_width = 8;
        *glyphs = file + sizeof(psf1_header);
    } else {
        psf2_header *header = (psf2_header *) file;
        *glyph_height = (uint8_t) header->glyph_height /*+ (8 - header->glyph_height % 8) % 8*/; //rounds to the nearest greater multiple of 8
        *glyph_width = (uint8_t) header->glyph_width + (8 - header->glyph_width % 8) % 8;
        *glyphs = file + sizeof(psf2_header);
        //*glyph_padding = (8 - header->glyph_width % 8) % 8;
        //*glyph_padding = *glyph_padding == 0 ? 1 : *glyph_padding;
    }
}

void set_tty_char_fg(tty_color_t new_color) {
    tty_color.foreground = new_color;
}

tty_color_t get_tty_char_fg() {
    return tty_color.foreground;
}

void set_tty_char_bg(tty_color_t new_color) {
    tty_color.background = new_color;
}

tty_color_t get_tty_char_bg() {
    return tty_color.background;
}

uint64_t get_tty_grid_height() {
    return tty_grid.height;
}

uint64_t get_tty_grid_width() {
    return tty_grid.width;
}

void tty_enable_bg() {
    tty_color.bg_transparent = false;
}

void tty_disable_bg() {
    tty_color.background = true;
}

inline void plot_pixel(uint64_t x, uint64_t y, tty_color_t pixel) {
    if (!tty_ready) {return;}
    uint32_t *loc = fb + y * tty_grid.cell_pitch * 4 + x * 4;
    *loc = pixel;
}

void launch_splashscreen() {
    uint16_t hue = 0;

    for (uint64_t y = 0; y < tty_height; y++) {
        for (uint64_t x = 0; x < tty_width; x++) {
            uint8_t r, g, b;
            hsv_to_rgb(hue % 360, 1, 1, &r, &g, &b);
            uint32_t rgb = r << 16 | g << 8 | b;
            plot_pixel(x, y, rgb);

            for (int i = 0; i < 2500; i++) {}
        }

        hue++;
    }
}

void tty_tem_line_number() {
    if (!tem_enabled) {
        return;
    }

    tty_color_t prev = get_tty_char_fg();
    set_tty_char_fg(GREY_COLOR);
    printf("%02d  ", tty_cursor.y);
    set_tty_char_fg(prev);
}

void tty_tem_open_bracket() {
    if (!tem_enabled) {
        return;
    }

    tty_color_t prev = get_tty_char_fg();
    set_tty_char_fg(RED_COLOR);
    printf("{\n", tty_cursor.y);
    set_tty_char_fg(prev);
}

void tty_tem_close_bracket() {
    if (!tem_enabled) {
        return;
    }

    tty_color_t prev = get_tty_char_fg();
    set_tty_char_fg(RED_COLOR);
    putchar_at('}', 0, tty_cursor.y + 1, tty_color.foreground, tty_color.background);
    set_tty_char_fg(prev);
}

void tty_tem_enable() {
    tem_enabled = true;
}

void tty_tem_disable() {
    tem_enabled = false;
}

//print a character to a specific grid location
void putchar_at(char c, uint32_t x, uint32_t y, tty_color_t fg, tty_color_t bg) {
    if (!tty_ready) {return;}

    if (c == '\t') {
        c = ' ';
    }

    uint8_t *glyph = glyphs + tty_grid.cell_height * (tty_grid.cell_width / 8) * c;

    for (int i = 0; i < tty_grid.cell_height; i++) {
        for (int j = 0; j < tty_grid.cell_width; j++) {
            if (*(glyph + i * tty_grid.cell_width / 8 + j / 8) >> (7 - j % 8) & 1) {
                plot_pixel(x + j, y + i, fg);
            } else if (!tty_color.bg_transparent) {
                plot_pixel(x + j, y + i, bg);
            }
        }
    }
}

void putchar(char c, tty_color_t fg, tty_color_t bg) {
    if (!tty_ready) {
        return;
    }

    //if we reached the end of line or the character to be displayed is a new line, move the cursor to the next line full left
    if (c == '\n' || tty_cursor.x == tty_grid.width) {
        tty_cursor.x = 0;

        if (tty_cursor.y < tty_grid.height) {
            tty_cursor.y++;
        } else {
            tty_clear();
            tty_cursor.x = 0;
            tty_cursor.y = 0;
        }
        
        tty_tem_line_number();
        //tty_tem_close_bracket();

        if (c == '\n') {
            return;
        }
    }

    putchar_at(c, tty_grid.cell_width * tty_cursor.x, tty_grid.cell_height * tty_cursor.y, fg, bg);
    tty_cursor.x++; //move the cursor 1 position right
}

/*
clears a grid cell
*/
void tty_clear_cell(uint32_t x, uint32_t y) {
    if (x >= tty_grid.width || y >= tty_grid.height) {
        return;
    }

    for (uint8_t i = 0; i < tty_grid.cell_height; i++) {
        for (uint8_t j = 0; j < tty_grid.cell_width; j++) {
            plot_pixel(x * tty_grid.cell_width + j, y * tty_grid.cell_height + i, tty_color.background);
        }
    }
}

void tty_backspace() {
    if (tty_cursor.x == 0 && !tem_enabled || tty_cursor.x == 4 && tem_enabled) {
        if (tem_enabled) {
            tty_clear_cell(3, tty_cursor.y);
            tty_clear_cell(2, tty_cursor.y);
            tty_clear_cell(1, tty_cursor.y);
            tty_clear_cell(0, tty_cursor.y);
        }

        if (tty_cursor.y > 0) {
            tty_cursor.x = tty_grid.width - 1;
            tty_cursor.y--;
        }
    } else {
        tty_cursor.x--;
    }

    tty_clear_cell(tty_cursor.x, tty_cursor.y);
}

void print_color(char *str, tty_color_t fg, tty_color_t bg) {
    if (!tty_ready) {return;}
    uint32_t len = strlen(str);

    for (int i = 0; i < len; i++) {
        char c = *(str + i);

        if (c >= 32 && c <= 126 || c == '\n' || c == '\t') {
            putchar(c, fg, bg);
        }
    }
}

void printf(const char *fmt, ...) {
    if (!tty_ready) {return;}
    va_list args;
    va_start(args, fmt);
    char buf[PRINTF_MAX_BUFFER];
    vsnprintf(buf, PRINTF_MAX_BUFFER, (char *) fmt, args);
    print_color(buf, tty_color.foreground, tty_color.background);
    va_end(args);
}

void draw_grid() {
    if (!tty_ready) {return;}

    //draws vertical lines
    for (int i = 0; i < tty_grid.width; i++) {
        for (int j = 0; j < tty_height; j++) {
            plot_pixel(i * tty_grid.cell_width, j, 0x555555);
        }
    }

    //draws horizontal lines
    for (int i = 0; i < tty_grid.height; i++) {
        for (int j = 0; j < tty_width; j++) {
            plot_pixel(j, i * tty_grid.cell_height, 0x555555);
        }
    }
}

//clear the screen
void tty_clear() {
    memclear(fb, tty_size);
    tty_cursor.x = 0;
    tty_cursor.y = 0;
}

//prints an error message and halts the cpu
void fail(char *str) {
    printf("-----[%s]-----\n", str);
    sys_hlt();
}

void cursor_blinkr() {
    static bool visible = false;

    if (!visible) {
        for (uint32_t i = 0; i < tty_grid.cell_height; i++) {
            for (uint32_t j = 0; j < tty_grid.cell_width; j++) {
                plot_pixel(tty_cursor.x * tty_grid.cell_width + j, tty_cursor.y * tty_grid.cell_height + i, RGB(200, 200, 200));
            }
        }

        visible = true;
    } else {
        for (uint32_t i = 0; i < tty_grid.cell_height; i++) {
            for (uint32_t j = 0; j < tty_grid.cell_width; j++) {
                plot_pixel(tty_cursor.x * tty_grid.cell_width + j, tty_cursor.y * tty_grid.cell_height + i, TTY_COLOR_BLACK);
            }
        }

        visible = false;
    }
}