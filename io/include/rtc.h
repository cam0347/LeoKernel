#pragma once
#include <include/types.h>
#include <include/date.h>
#define CMOS_SELECT_PORT 0x70 //register number and nmi control
#define CMOS_DATA_PORT 0x71
#define CMOS_SECONDS_REGISTER 0x00
#define CMOS_MINUTES_REGISTER 0x02
#define CMOS_HOURS_REGISTER 0x04
#define CMOS_WEEK_DAY_REGISTER 0x06
#define CMOS_MONTH_DAY_REGISTER 0x07
#define CMOS_MONTH_REGISTER 0x08
#define CMOS_YEAR_REGISTER 0x09
#define CMOS_CENTURY_REGISTER 0x32
#define CMOS_STATUS_A_REGISTER 0x0A
#define CMOS_STATUS_B_REGISTER 0x0B
#define RTC_BASE_YEAR 2000

uint8_t cmos_read_register(uint8_t reg);
uint8_t cmos_write_register(uint8_t reg, uint8_t data);
bool cmos_update_in_progress();
timedate_t rtc_read_timedate();