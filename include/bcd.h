#pragma once
#define BCD_TO_BINARY_BYTE(n) (((n & 0x0F) + (((n & 0x70) / 16) * 10)) | (n & 0x80))