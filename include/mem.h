#pragma once
#include <include/types.h>

uint64_t memcpy(void *dst, void *src, uint64_t size);
uint64_t memmove(void *dst, void *src, uint64_t size);
void memset(void *dst, uint64_t size, uint8_t value);
void memclear(void *, uint64_t);
void reverse_endianess(void *, uint64_t);
bool memcmp(void *, void *, uint64_t);