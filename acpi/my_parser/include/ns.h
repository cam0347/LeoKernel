#pragma once
#include <include/types.h>
#define HAS_NEXT(ns_node) (ns_node->next != null)

enum {
	ACPI_OBJECT_TYPE_NULL         = 0x0001,
	ACPI_OBJECT_TYPE_ALIAS        = 0x0002,
	ACPI_OBJECT_TYPE_EVENT        = 0x0003,
	ACPI_OBJECT_TYPE_FIELD        = 0x0004,
	ACPI_OBJECT_TYPE_INDEX_FIELD  = 0x0005,
	ACPI_OBJECT_TYPE_BANK_FIELD   = 0x0006,
	ACPI_OBJECT_TYPE_METHOD       = 0x0007,
	ACPI_OBJECT_TYPE_MUTEX        = 0x0008,
	ACPI_OBJECT_TYPE_NAME         = 0x0009,
	ACPI_OBJECT_TYPE_OP_REGION    = 0x000A,
	ACPI_OBJECT_TYPE_DEVICE       = 0x00B0,
	ACPI_OBJECT_TYPE_POWER_RES    = 0x00C0,
	ACPI_OBJECT_TYPE_PROCESSOR    = 0x00D0,
	ACPI_OBJECT_TYPE_THERMAL_ZONE = 0x00E0,
	ACPI_OBJECT_TYPE_SCOPE        = 0x00F0
};

enum {
	ACPI_DATA_TYPE_NULL         = 0x0100,
	ACPI_DATA_TYPE_BUFFER       = 0x0200,
	ACPI_DATA_TYPE_BUFFER_FIELD = 0x0300,
	ACPI_DATA_TYPE_PACKAGE      = 0x0400,
	ACPI_DATA_TYPE_STRING       = 0x0500,
	ACPI_DATA_TYPE_INTEGER      = 0x0600,
	ACPI_DATA_TYPE_NAME         = 0x0700,
	ACPI_DATA_TYPE_REFERENCE    = 0x0800
};

typedef struct {
	char prefix[16];
	char value[64];
} acpi_ns_name_t;

union acpi_ns_object;
struct acpi_ns_node;

//defines a data type
typedef union {
	uint16_t type;
	struct {
		uint16_t type;
		uint32_t size;
		uint8_t *ptr;
	} buffer;
	struct {
		uint16_t type;
		uint32_t size;
		uint8_t offset;
		uint8_t *ptr;
	} field;
	struct {
		uint16_t type;
		uint64_t value;
	} integer;
	struct {
		uint16_t type;
		char *value;
	} string;
	struct {
		uint16_t type;
		acpi_ns_name_t path;
	} name;
	struct {
		uint16_t type;
		uint32_t count;
		acpi_ns_name_t *data;
	} package;
	struct {
		uint16_t type;
		void *refof;
		int index;
		int flags;
	} reference;
} acpi_ns_data_t;

//defines a node's object structure
typedef union acpi_ns_object {
	uint16_t type;
	struct {
		uint16_t type;
		struct acpi_ns_node *node;
	} alias;
	struct {
		uint16_t type;
		void *adr;
	} device;
	struct {
		uint16_t type;
		uint8_t *start;
		uint8_t *end;
		uint8_t flags;
		uint8_t argc;
	} method;
	struct {
		uint16_t type;
		uint8_t space;
		uint64_t offset;
		uint64_t length;
		void *adr;
	} region;
	struct {
		uint16_t type;
		uint8_t system_level;
		uint32_t resource_order;
	} power;
	struct {
		uint16_t type;
		uint8_t proc_id;
		uint32_t pblk_addr;
		uint8_t pblk_len;
	} cpu;
	struct {
		uint16_t type;
		uint8_t syncflags;
	} mutex;
	struct {
		uint16_t type;
		acpi_ns_data_t *data;
	} name;
	struct {
		uint16_t type;
		uint8_t flags;
		uint8_t attrib;
		uint32_t offset;
		uint32_t length;
		union acpi_ns_object *parent;
	} field;
	struct {
		uint16_t type;
		uint8_t flags;
		uint8_t attrib;
		uint32_t offset;
		uint32_t length;
		union acpi_ns_object *index;
		union acpi_ns_object *data;
	} indexfield;
	struct {
		uint16_t type;
		uint8_t flags;
		uint8_t attrib;
		uint32_t offset;
		uint32_t length;
		uint64_t value;
		union acpi_ns_object *region;
		union acpi_ns_object *bank;
	} bankfield;
} acpi_ns_object_t;

//defines a node in the namespace hierarchy
typedef struct acpi_ns_node {
    char name[4];
    struct acpi_ns_node *next;
    struct acpi_ns_node *child;
    struct acpi_ns_node *parent;
    uint8_t type;
    acpi_ns_object_t object;
} acpi_ns_node_t;

bool acpi_ns_init();
acpi_ns_node_t *acpi_ns_create_node(char *name);
void acpi_ns_append_node(acpi_ns_node_t *parent, acpi_ns_node_t *child);
acpi_ns_node_t *acpi_ns_find_node(char *name);
acpi_ns_node_t *acpi_ns_root();