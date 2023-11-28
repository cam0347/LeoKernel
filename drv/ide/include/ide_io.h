#pragma once
#include <include/types.h>
#include <drv/ide/include/ide.h>

void ide_wait(ide_bus_t *bus);
void ide_poll(ide_bus_t *bus);
bool ide_check_error(ide_bus_t *bus);
uint16_t ide_read(uint32_t bar, uint8_t reg);
void ide_write(uint32_t bar, uint8_t reg, uint16_t data);
bool ide_init_transaction(ide_bus_t *bus, uint64_t address, uint16_t sectors);
bool ide_port_read_data(ide_bus_t *bus, uint64_t address, uint16_t sectors, void *data);
bool ide_port_write_data(ide_bus_t *bus, uint64_t address, uint16_t sectors, void *data);
void ide_select_drive(ide_bus_t *bus, ide_drive_select_t drive);
void ide_command(ide_bus_t *bus, uint8_t command);
uint8_t ide_status(ide_bus_t *bus);
void ide_reset(ide_bus_t *bus);