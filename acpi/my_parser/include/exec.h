#pragma once
#include <include/types.h>
#include <acpi/my_parser/include/vm.h>
#define AML_EXTOP_PREFIX 0x5B
#define IS_OPCODE_EXT(opcode) (opcode & 0xFF00 != 0)

bool parse_table(void *table);
void acpi_vm_fetch(acpi_vm_state_t *state, uint16_t *opcode, void **args);
void acpi_vm_exec(uint16_t opcode, void *args, uint32_t *args_size);