#pragma once
#include <include/types.h>

typedef struct {
    uint8_t foo;
} ahci_controller_t;

typedef enum {
    fis_reg_host_to_dev = 0x27,   //register fis, host to device
    fis_reg_dev_to_host = 0x34,   //register fis, device to host
    fis_data_bi = 0x46,           //data fis, bidirectional
    fis_pio_setup = 0x5F          //pio setup fis
} ahci_fis_type_t;

/* register fis, host to device, used to send commands to a device */
typedef struct {
	uint8_t fis_type;	   // fis_reg_host_to_dev
	uint8_t pmport: 4;     // port multiplier
	uint8_t rsv0: 3;	   // reserved
	uint8_t c: 1;		   // 1: command, 0: control
	uint8_t command;	   // command register
	uint8_t feature_lo;    // feature register, 7:0
	uint8_t lba_lo;	       // LBA low register, 7:0
	uint8_t lba_mid;	   // LBA mid register, 15:8
	uint8_t lba_hi;	       // LBA high register, 23:16
	uint8_t device;	       // device register
	uint8_t lba3;		   // LBA register, 31:24
	uint8_t lba4;		   // LBA register, 39:32
	uint8_t lba5;		   // LBA register, 47:40
	uint8_t feature_hi;    // feature register, 15:8
	uint8_t count_lo;	   // count register, 7:0
	uint8_t count_hi;	   // count register, 15:8
	uint8_t icc;		   // isochronous command completion
	uint8_t control;	   // control register
	uint8_t rsv1[4];	   // reserved
} ahci_fis_reg_h2d_t;

/* register fis, device to host, used to notify the host about a change in the device registers */
typedef struct {
	uint8_t fis_type;      // fis_reg_dev_to_host
	uint8_t pmport: 4;     // port multiplier
	uint8_t rsv0: 2;       // reserved
	uint8_t i: 1;          // interrupt bit
	uint8_t rsv1: 1;       // reserved
	uint8_t status;        // status register
	uint8_t error;         // error register
	uint8_t lba_low;       // LBA low register, 7:0
	uint8_t lba_mid;       // LBA mid register, 15:8
	uint8_t lba_hi;        // LBA high register, 23:16
	uint8_t device;        // device register
	uint8_t lba3;          // LBA register, 31:24
	uint8_t lba4;          // LBA register, 39:32
	uint8_t lba5;          // LBA register, 47:40
	uint8_t rsv2;          // reserved
	uint8_t count_lo;      // count register, 7:0
	uint8_t count_hi;      // count register, 15:8
	uint8_t rsv3[2];       // reserved
	uint8_t rsv4[4];       // reserved
} ahci_fis_reg_d2h_t;

/* data fis, bidirectional, used to transfer data to and from a device */
typedef struct {
	uint8_t fis_type;	   // fis_data_bi
	uint8_t pmport: 4;	   // port multiplier
	uint8_t rsv0: 4;	   // reserved
	uint8_t rsv1[2];	   // reserved
	uint32_t data[];	   // payload, this field varies in length
} ahci_fis_data_t;

/* pio setup fis, used by the device to tell the host it's about to send pio data */
typedef struct {
	uint8_t fis_type;	   // fis_pio_setup
	uint8_t pmport: 4;	   // port multiplier
	uint8_t rsv0: 1;	   // reserved
	uint8_t d: 1;		   // data transfer direction, 1 - device to host
	uint8_t i: 1;		   // interrupt bit
	uint8_t rsv1: 1;
	uint8_t status;		   // status register
	uint8_t error;		   // error register
	uint8_t lba_low;	   // LBA low register, 7:0
	uint8_t lba_mid;	   // LBA mid register, 15:8
	uint8_t lba_hi;		   // LBA high register, 23:16
	uint8_t device;		   // device register
	uint8_t lba3;		   // LBA register, 31:24
	uint8_t lba4;		   // LBA register, 39:32
	uint8_t lba5;		   // LBA register, 47:40
	uint8_t rsv2;		   // reserved
	uint8_t count_lo;	   // count register, 7:0
	uint8_t count_hi;	   // count register, 15:8
	uint8_t rsv3;		   // reserved
	uint8_t e_status;	   // new value of status register
	uint16_t tc;		   // transfer count
	uint8_t rsv4[2];	   // reserved
} ahci_fis_pio_setup_t;