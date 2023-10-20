#include <acpi/my_parser/include/ns.h>
#include <include/types.h>
#include <mm/include/kmalloc.h>
#include <include/mem.h>
#include <include/string.h>
#include <acpi/my_parser/include/vm.h>

acpi_ns_node_t *root;
bool acpi_ns_ready = false;

acpi_ns_node_t *acpi_ns_root() {
    return root;
}

bool acpi_ns_init() {
    acpi_ns_node_t *node;
    acpi_ns_object_t obj;

    if ((node = acpi_ns_create_node("\\___"))) {
        root = node;
        acpi_ns_ready = true;
        acpi_vm_state()->scope = node;
    } else {
        return false;
    }

    //create \_SB_
    if ((node = acpi_ns_create_node("_SB_"))) { //device/bus objects
        acpi_ns_append_node(root, node);
    } else {
        return false;
    }

    //create \_SI_
    if ((node = acpi_ns_create_node("_SI_"))) { //system indicator objects
        acpi_ns_append_node(root, node);
    } else {
        return false;
    }

    //create \_TZ_
    if ((node = acpi_ns_create_node("_TZ_"))) { //thermal zone objects, deprecated but kept for compatibility
        acpi_ns_append_node(root, node);
    } else {
        return false;
    }

    //create \_PR_
    if ((node = acpi_ns_create_node("_PR_"))) { //processor objects, deprecated but kept for compatibility
        acpi_ns_append_node(root, node);
    } else {
        return false;
    }

    //create \_GPE
    if ((node = acpi_ns_create_node("_GPE"))) { //general events
        acpi_ns_append_node(root, node);
    } else {
        return false;
    }

    //create \_OS_
    if ((node = acpi_ns_create_node("_OS_"))) {
        acpi_ns_append_node(root, node);
    } else {
        return false;
    }

    //create \_REV
    if ((node = acpi_ns_create_node("_REV"))) {
        acpi_ns_append_node(root, node);
    } else {
        return false;
    }

    //create \_OSI
    if ((node = acpi_ns_create_node("_OSI"))) {
        acpi_ns_append_node(root, node);
    } else {
        return false;
    }
}

acpi_ns_node_t *acpi_ns_create_node(char *name) {
    acpi_ns_node_t *node = (acpi_ns_node_t *) kmalloc(sizeof(acpi_ns_node_t));
    if (!node) return null;
    memclear(node, sizeof(acpi_ns_node_t));
    memcpy(node->name, name, 4);
    return node;
}

void acpi_ns_append_node(acpi_ns_node_t *parent, acpi_ns_node_t *child) {
    if (!acpi_ns_ready || !parent || !child) return;
    child->parent = parent;
    child->next = parent->child;
    parent->child = child;
}

void acpi_ns_init_node(acpi_ns_node_t *node, acpi_ns_node_t *next, acpi_ns_node_t *child, acpi_ns_node_t *parent, uint8_t type) {
    if (!node) return;
    node->next = next;
    node->child = child;
    node->parent = parent;
}

/*
Search recursively a node in the namespace hiearchy.
If found returns the pointer to that node, otherwise null.
*/
acpi_ns_node_t *acpi_ns_find_node(char *path) {
    acpi_ns_node_t *scope = acpi_vm_state()->scope;

    if (*path == '\\') {
        scope = root;
        path++;
    }

    while(*path == '^') {
        if (scope->parent) {
            scope = scope->parent;
        }

        path++;
    }

    uint32_t path_length = strlen(path);

    if (path_length < 4) {
        return null;
    }

    while(scope) {
        acpi_ns_node_t *node = scope->child;

        while(node) {
            if (strncmp(node->name, path, 4) == 0) {
                if (path_length > 4 && path + path_length == '.') {
                    return acpi_ns_find_node(path + path_length + 1);
                } else {
                    return node;
                }
            } else {
                node = node->next;
            }
        }

        scope = scope->parent;
    }

    return null;
}