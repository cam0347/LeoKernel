#pragma once
#include <tty/include/tty.h>
#define PSF1_HEADER_MAGIC 0x0436
#define PSF2_HEADER_MAGIC 0x864AB572
#define PSF1_MODE512 0x01 //if set, there will be 512 glyphs, if not there will be only 256
#define PSF_MODEHASTAB 0x02 //if set, this file has a unicode table
#define PSF_MODESEQ 0x04 //equivalent to PSF_MODEHASTAB
#define PSF2_HAS_UNICODE_TABLE 0x01 //if set this file has a unicode table

typedef struct {
    uint16_t magic;
    uint8_t mode;
    uint8_t character_size;
} psf1_header;

typedef struct {
    uint32_t magic;
    uint32_t version; //should always be 0
    uint32_t header_size; //usually 32
    uint32_t flags;
    uint32_t num_glyphs;
    uint32_t bytes_per_glyph;
    uint32_t glyph_height;
    uint32_t glyph_width;
} psf2_header;