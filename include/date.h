#pragma once
#include <include/types.h>

typedef struct {
    uint8_t millisecond;
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
} time_t;

typedef struct {
    uint8_t week_day;
    uint8_t month_day;
    uint8_t month;
    uint16_t year;
    uint16_t century;
} date_t;

typedef struct {
    time_t time;
    date_t date;
} timedate_t;