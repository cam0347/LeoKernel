//#include <acpi/lai/include/lai/host.h>
#include <include/types.h>
#include <mm/include/kmalloc.h>
#include <tty/include/tty.h>
#include <acpi/include/acpi_parser.h>
#include <io/include/port_io.h>

void *laihost_malloc(uint64_t size) {
    return kmalloc(size);
}

void *laihost_realloc(void *ptr, uint64_t newsize, uint64_t oldsize) {
    if (!ptr) {
        return kmalloc(newsize);
    } else {
        return krealloc(ptr, newsize);
    }
}

void laihost_free(void *ptr, uint64_t size) {
    kfree(ptr);
}

__attribute__((weak)) void laihost_log(int level, const char *str) {
    printf("LAI log: %s\n", str);
}

__attribute__((weak, noreturn)) void laihost_panic(const char *) {
    printf("LAI panic\n");
    while(true);
}

__attribute__((weak)) void *laihost_scan(const char *name, uint64_t n) {
    return acpi_locate_table((char *) name, n);
}

__attribute__((weak)) void *laihost_map(uint64_t, uint64_t);
__attribute__((weak)) void laihost_unmap(void *, uint64_t);
__attribute__((weak)) void laihost_outb(uint16_t port, uint8_t data) {
    outb(port, data);
}

__attribute__((weak)) void laihost_outw(uint16_t port, uint16_t data) {
    outw(port, data);
}

__attribute__((weak)) void laihost_outd(uint16_t port, uint32_t data) {
    outl(port, data);
}

__attribute__((weak)) uint8_t laihost_inb(uint16_t port) {
    return inb(port);
}

__attribute__((weak)) uint16_t laihost_inw(uint16_t port) {
    return inw(port);
}

__attribute__((weak)) uint32_t laihost_ind(uint16_t port) {
    return inl(port);
}