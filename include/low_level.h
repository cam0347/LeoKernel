#pragma once
#include <include/types.h>

void sys_hlt();
void disable_int();
void enable_int();
uint64_t get_cr3();
uint64_t get_cr2();
void cpuid(uint32_t leaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);
void get_msr(uint32_t msr, uint32_t *lo, uint32_t *hi);
void set_msr(uint32_t msr, uint32_t lo, uint32_t hi);