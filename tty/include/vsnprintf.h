#pragma once
#include <include/types.h>
#include <include/stdargs.h>

char *num_fmt(uint64_t i, uint32_t base, uint32_t padding, char pad_with, bool upper, int len);
void vsnprintf(char *buf, uint64_t len, char *fmt, va_list arg);