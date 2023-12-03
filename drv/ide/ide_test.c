#include <include/types.h>
#include <drv/ide/include/ide_wrapper.h>
#include <mm/include/obj_alloc.h>
#include <mm/include/kmalloc.h>
#include <include/mem.h>

#define TEST_BUFFER_SIZE 512

extern pool_t ide_devices_pool_id;
extern uint16_t ide_devices_pool_last_ind;

bool ide_test() {
    char *test_str = "Hello World";
    ide_write_wrapper(0, 100, 1, test_str);

    char buffer[512];
    memclear(buffer, 512);

    ide_read_wrapper(0, 100, 1, buffer);

    for (uint8_t i = 0; i < 50; i++) {
        printf("%c", buffer[i]);
    }

    return true;
}