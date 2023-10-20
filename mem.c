#include <include/mem.h>
#include <include/types.h>

uint64_t memcpy(void *dst, void *src, uint64_t size) {
    for (uint64_t i = 0; i < size; i++) {
        *(char *)(dst + i) = *(char *)(src + i);
    }

    return size;
}

uint64_t memmove(void *dst, void *src, uint64_t size) {
    for (uint64_t i = 0; i < size; i++) {
        *(char *)(dst + i) = *(char *)(src + i);
        *(char *)(src + i) = 0x00;
    }

    return size;
}

void memset(void *dst, uint64_t size, uint8_t value) {
    for (uint64_t i = 0; i < size; i++) {
        *((uint8_t *)(dst + i)) = value;
    }
}

void memclear(void *dst, uint64_t size) {
    memset(dst, size, 0);
}

bool memcmp(void *buffer1, void *buffer2, uint64_t length) {
    for (uint64_t i = 0; i < length; i++) {
        if (*(uint8_t *)(buffer1 + i) != *(uint8_t *)(buffer2 + i)) {
            return false;
        }
    }

    return true;
}

void reverse_endianess(void *ptr, uint64_t length) {
    if (length < 2) {
        return;
    }

    uint8_t tmp;

    for (uint32_t i = 0; i < length / 2; i++) {
        tmp = *(uint8_t *)(ptr + i);
        *(uint8_t *)(ptr + i) = *(uint8_t *)(ptr + (length - 1) - i);
        *(uint8_t *)(ptr + (length - 1) - i) = tmp;
    }
}