#include <drv/ide/include/ide.h>
#include <drv/ide/include/ide_io.h>
#include <drv/ide/include/ide_interface.h>
#include <include/types.h>
#include <mm/include/obj_alloc.h>
#include <include/mem.h>
#include <io/include/port_io.h>

extern pool_t ide_controllers_pool_id; //defined in ide.c

bool ide_int_get_drive(ide_device_id_t id, ide_bus_t **bus, ide_device_t **dev) {
    ide_controller_t *controller;
    ide_bus_t *_bus;
    ide_device_t *_dev;

    if (!obj_pool_get(ide_controllers_pool_id, (void *) &controller, id.controller)) {
        return false;
    }

    if (!controller->pci_dev) {
        //the requested controller doesn't exist
        return false;
    }
    
    _bus = id.bus == primary ? &controller->primary : &controller->secondary;
    _dev = id.drive == master ? &_bus->master : &_bus->slave;

    if (!_dev->exist) {
        return false; //the requested device doesn't exist
    }

    if (bus != null) {
        *bus = _bus;
    }

    if (dev != null) {
        *dev = _dev;
    }

    return true;
}

bool ide_int_read(ide_device_id_t id, uint64_t address, uint16_t sectors, void *buffer) {
    if (!buffer) {
        return false;
    }

    ide_bus_t *bus;

    if (!ide_int_get_drive(id, &bus, null)) {
        return false;
    }

    ide_select_drive(bus, id.drive);
    return ide_port_read_data(bus, address, sectors, buffer);
}

bool ide_int_write(ide_device_id_t id, uint64_t address, uint16_t sectors, void *data) {
    if (!data) {
        return false;
    }

    ide_bus_t *bus;

    if (!ide_int_get_drive(id, &bus, null)) {
        return false;
    }

    ide_select_drive(bus, id.drive);
    return ide_port_write_data(bus, address, sectors, data);
}