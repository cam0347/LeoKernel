#pragma once
#include <include/types.h>
#define PIT_CHANNEL0_DATA_PORT 0x40 //channel 0 is connected to irq0
#define PIT_CHANNEL1_DATA_PORT 0x41 //channel 1 is unused and may not even exist
#define PIT_CHANNEL2_DATA_PORT 0x42 //channel 2 is connected to the pc speaker
#define PIT_MODE_COMMAND_REGISTER_PORT 0x43
#define PIT_IRQ 2

/* use these to compose a command for the pit */

#define PIT_COMM_BIN 0
#define PIT_COMM_BCD 1
#define PIT_COMM_INT 0 << 1
#define PIT_COMM_HARDWARE_RETRIGGERABLE 1 << 1
#define PIT_COMM_RATE_GENERATOR 2 << 1
#define PIT_COMM_SQUARE_WAVE 3 << 1
#define PIT_COMM_SW_TRIGGER_STROBE 4 << 1
#define PIT_COMM_HW_TRIGGER_STROBE 5 << 1
#define PIT_COMM_LOBYTE 1 << 4
#define PIT_COMM_HIBYTE 2 << 4
#define PIT_COMM_LOBYTE_HIBYTE 3 << 4
#define PIT_COMM_CHANNEL0 0 << 6
#define PIT_COMM_CHANNEL1 1 << 6
#define PIT_COMM_CHANNEL2 2 << 6
#define PIT_COMM_READ_BACK 3 << 6

void init_pit();
void pit_command(uint8_t command);
void pit_write(uint8_t channel, uint8_t data);
void pit_run();
void pit_stop();