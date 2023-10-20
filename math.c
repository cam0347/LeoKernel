#include <include/math.h>
#include <include/types.h>

double ceil(double n) {
    if (n == (int) n) {
        return n;
    } else {
        return (double)((int) n + 1);
    }
}

double floor(double n) {
    return (float)(int) n;
}

uint32_t pow(uint32_t base, uint32_t exp) {
    uint32_t ret = base;

    if (exp == 0) {
        return 1;
    }

    for (int i = 0; i < exp - 1; i++) {
        ret *= base;
    }

    return ret;
}

float abs(float n) {
    return n >= 0 ? n : -n;
}

uint64_t long_max(uint64_t n1, uint64_t n2) {
    return n1 > n2 ? n1 : n2;
}

uint64_t long_min(uint64_t n1, uint64_t n2) {
    return n1 < n2 ? n1 : n2;
}

uint32_t int_max(uint32_t n1, uint32_t n2) {
    return (uint32_t) long_max((uint64_t) n1, (uint64_t) n2);
}

uint32_t int_min(uint32_t n1, uint32_t n2) {
    return (uint32_t) long_min((uint64_t) n1, (uint64_t) n2);
}

uint16_t short_max(uint16_t n1, uint16_t n2) {
    return (uint16_t) long_max((uint64_t) n1, (uint64_t) n2);
}

uint16_t short_min(uint16_t n1, uint16_t n2) {
    return (uint16_t) long_min((uint64_t) n1, (uint64_t) n2);
}

uint8_t char_max(uint8_t n1, uint8_t n2) {
    return (uint8_t) long_max((uint64_t) n1, (uint64_t) n2);
}

uint8_t char_min(uint8_t n1, uint8_t n2) {
    return (uint8_t) long_min((uint64_t) n1, (uint64_t) n2);
}

double fmod(double a, double b) {
    if (b == 0.0) {
        return 0.0;
    }

    double q = a / b;
    double wholePart = (double)((long long) q);
    double r = a - wholePart * b;

    return r;
}

static uint32_t next_rand = 1;
uint32_t rand(void) {
    next_rand = next_rand * 1103515245 + 12345;
    return (uint32_t) (next_rand / 65536) % 32768;
}

void srand(uint32_t seed) {
    next_rand = seed;
}