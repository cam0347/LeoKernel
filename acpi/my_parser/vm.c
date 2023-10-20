#include <include/types.h>
#include <acpi/my_parser/include/vm.h>
#include <acpi/my_parser/include/ns.h>
#include <tty/include/tty.h>
#include <include/low_level.h>

static acpi_vm_state_t state;

acpi_vm_state_t *acpi_vm_state() {
    return &state;
}

void acpi_vm_panic_wrapper(char *file, uint32_t line, const char *fmt, ...) {
    printf("\n..........................\n");
    printf("At %s:%d\n", file, line);
    acpi_vm_panic(fmt);
}

void acpi_vm_panic(const char *fmt, ...) {
    printf("[ ACPI VM panic ]\n");
    printf(fmt);
    state.pc = state.aml + state.size; //move the program counter at the end so that the interpreter stops
    sys_hlt();
}