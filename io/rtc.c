#include <include/types.h>
#include <io/include/rtc.h>
#include <include/date.h>
#include <io/include/port_io.h>
#include <int/include/int.h>
#include <include/bcd.h>

/*
Nothing to do with rtc but the same port used as a CMOS select register contains the NMI enabled bit.
Defined in int.c
*/
extern bool nmi_enabled;

uint8_t cmos_read_register(uint8_t reg) {
    outb(CMOS_SELECT_PORT, reg | !nmi_enabled << 7); //nmi enable bit has to be inverted (0 enabled, 1 disabled)
    for (uint32_t i = 0; i < 1000000; i++);
    return inb(CMOS_DATA_PORT); //this resets 0x70 to 0x0D
}

//unused
uint8_t cmos_write_register(uint8_t reg, uint8_t data) {
    outb(CMOS_SELECT_PORT, reg | nmi_enabled << 7); //bit 7 controls NMI
    for (uint32_t i = 0; i < 1000000; i++);
    outb(CMOS_DATA_PORT, data);
}

//returns true when the cmos is updating
bool cmos_update_in_progress() {
    return (bool)((cmos_read_register(CMOS_STATUS_A_REGISTER) >> 7 & 1));
}

//returns rtc's time and date
timedate_t rtc_read_timedate() {
    timedate_t ret;
    uint8_t status_b = cmos_read_register(CMOS_STATUS_B_REGISTER);
    bool h24 = status_b >> 1 & 1;
    bool binary = status_b >> 2 & 1;
    while(cmos_update_in_progress()); //wait for the eventual update to complete

    //time
    ret.time.millisecond = 0;
    uint8_t second = cmos_read_register(CMOS_SECONDS_REGISTER);
    uint8_t minute = cmos_read_register(CMOS_MINUTES_REGISTER);
    uint8_t hour = cmos_read_register(CMOS_HOURS_REGISTER);

    //date
    uint8_t week_day = cmos_read_register(CMOS_WEEK_DAY_REGISTER);
    uint8_t month_day = cmos_read_register(CMOS_MONTH_DAY_REGISTER);
    uint8_t month = cmos_read_register(CMOS_MONTH_REGISTER);
    uint8_t year = cmos_read_register(CMOS_YEAR_REGISTER);
    uint16_t century = cmos_read_register(CMOS_CENTURY_REGISTER);

    if (!binary) {
        second = BCD_TO_BINARY_BYTE(second);
        minute = BCD_TO_BINARY_BYTE(minute);
        hour = BCD_TO_BINARY_BYTE(hour);
        month_day = BCD_TO_BINARY_BYTE(month_day);
        month = BCD_TO_BINARY_BYTE(month);
        year = BCD_TO_BINARY_BYTE(year);
        century = BCD_TO_BINARY_BYTE(century);
    }

    ret.time.second = second;
    ret.time.minute = minute;
    ret.time.hour = hour;
    ret.date.week_day = week_day;
    ret.date.month_day = month_day;
    ret.date.month = month;
    ret.date.year = RTC_BASE_YEAR + year;
    ret.date.century = century;

    return ret;
}