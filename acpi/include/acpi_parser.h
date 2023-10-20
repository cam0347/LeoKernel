#pragma once
#include <include/types.h>
#include <include/bootp.h>
#include <acpi/include/acpi.h>

bool init_acpi(struct leokernel_boot_params bootp);
void *acpi_locate_table(char *signature, uint32_t n);
void parse_fadt(void *fadt_addr);
void parse_madt(void *madt_addr);
void parse_dsdt(void *dsdt_addr);
bool validate_checksum(acpi_table_header *header);
void switch_madt_entry(void *entry, acpi_madt_entry_type type);