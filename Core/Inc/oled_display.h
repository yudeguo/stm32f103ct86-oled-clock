#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H
#include <stdint.h>

struct date_info_t
{
    uint8_t week;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint16_t year;
};

void set_current_time(struct date_info_t *date);
void display_smart_clock();
void display_wait_check_time();

#endif //OLED_DISPLAY_H
