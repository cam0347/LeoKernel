#pragma once
#include <include/types.h>
#include <drv/ide/include/ide.h>

typedef struct {
    uint8_t controller;
    ide_bus_select_t bus;
    ide_drive_select_t drive;
} ide_device_id_t;

bool ide_int_get_drive(ide_device_id_t id, ide_bus_t **bus, ide_device_t **dev);
bool ide_int_read(ide_device_id_t id, uint64_t address, uint16_t sectors, void *buffer);
bool ide_int_write(ide_device_id_t id, uint64_t address, uint16_t sectors, void *data);