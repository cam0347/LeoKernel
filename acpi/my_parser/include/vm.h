#pragma once
#include <include/types.h>
#include <acpi/my_parser/include/ns.h>

//defines the state of the vm
typedef struct {
    void *aml; //pointer to the base address of the aml code being executed
    void *pc;
    uint32_t size;
    acpi_ns_node_t *scope; //pointer to the current execution scope
} acpi_vm_state_t;

acpi_vm_state_t *acpi_vm_state();
void acpi_vm_panic(const char *fmt, ...);
void acpi_vm_panic_wrapper(char *file, uint32_t line, const char *fmt, ...);