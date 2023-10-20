#pragma once
#include <include/types.h>

double ceil(double n);
double floor(double n);
uint32_t pow(uint32_t base, uint32_t exp);
float abs(float n);
uint64_t long_max(uint64_t n1, uint64_t n2);
uint64_t long_max(uint64_t n1, uint64_t n2);
uint32_t int_max(uint32_t n1, uint32_t n2);
uint32_t int_min(uint32_t n1, uint32_t n2);
uint16_t short_max(uint16_t n1, uint16_t n2);
uint16_t short_min(uint16_t n1, uint16_t n2);
uint8_t char_max(uint8_t n1, uint8_t n2);
uint8_t char_min(uint8_t n1, uint8_t n2);
double fmod(double a, double b);
uint32_t rand(void);
void srand(uint32_t seed);