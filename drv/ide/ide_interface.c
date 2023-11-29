#include <drv/ide/include/ide.h>
#include <drv/ide/include/ide_io.h>
#include <drv/ide/include/ide_interface.h>
#include <include/types.h>
#include <mm/include/obj_alloc.h>
#include <include/mem.h>
#include <io/include/port_io.h>

extern pool_t ide_controllers_pool_id;         //defined in ide_setup.c
extern uint8_t ide_controllers_pool_last_ind;  //defined in ide_setup.c

/* given a ide device id, return the bus and device objects or return false */
bool ide_int_get_drive(ide_device_id_t id, ide_bus_t **bus, ide_device_t **dev) {
    ide_controller_t *found_controller;
    ide_bus_t *found_bus;
    ide_device_t *found_dev;

    if (id.controller > ide_controllers_pool_last_ind) {
        return false;
    }

    if (!obj_pool_get(ide_controllers_pool_id, (void *) &found_controller, id.controller)) {
        return false;
    }

    //if this controller have a null pointer to its pci device, it doesn't exist
    if (!found_controller->pci_dev) {
        return false;
    }
    
    found_bus = id.bus == primary ? &found_controller->primary : &found_controller->secondary;
    found_dev = id.drive == master ? &found_bus->master : &found_bus->slave;

    if (!found_dev->exist) {
        return false;
    }

    if (bus != null) {
        *bus = found_bus;
    }

    if (dev != null) {
        *dev = found_dev;
    }

    return true;
}

/* read some sectors from a drive */
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

/* write some sectors to a drive */
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