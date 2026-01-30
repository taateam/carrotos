#include <stdbool.h>
#include <stdint.h>
int cmos_read(uint8_t reg) {
    outb(0x70, reg);
    return inb(0x71);
}
int bcd_to_bin(int bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}
struct rtc_time {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
};
struct rtc_time rtc_get_time() {
    struct rtc_time t;

    
    int sec  = cmos_read(0x00);
    int min  = cmos_read(0x02);
    int hour = cmos_read(0x04);
    int day  = cmos_read(0x07);
    int mon  = cmos_read(0x08);
    int year = cmos_read(0x09);
    int century = cmos_read(0x32); 

    
    sec  = bcd_to_bin(sec);
    min  = bcd_to_bin(min);
    hour = bcd_to_bin(hour);
    day  = bcd_to_bin(day);
    mon  = bcd_to_bin(mon);
    year = bcd_to_bin(year);
    century = bcd_to_bin(century);

    // full year
    t.year = century * 100 + year;
    t.month = mon;
    t.day = day;
    t.hour = hour;
    t.minute = min;
    t.second = sec;

    return t;
}
