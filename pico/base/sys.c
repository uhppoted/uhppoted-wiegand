#include <stdio.h>

#include "hardware/clocks.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"

#include "../include/cli.h"
#include "../include/wiegand.h"

const char *MODES[] = {
    "UNKNOWN",
    "READER",
    "EMULATOR",
};

void sys_start() {
    char s[64];
    uint32_t hz = clock_get_hz(clk_sys);

    snprintf(s, sizeof(s), "%6s %d", "CLOCK", hz);
    tx(s);
}

void sys_ok() {
    char s[64];

    snprintf(s, sizeof(s), "%-6s %-8s %s", "SYS", MODES[mode], "OK");
    tx(s);
}

// NTS: seems like this needs to be a 'long lived struct' for RTC
datetime_t NOW = {
    .year = 2022,
    .month = 11,
    .day = 24,
    .hour = 21,
    .min = 44,
    .sec = 13,
};

void sys_settime(char *t) {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    int rc = sscanf(t, "%04d-%02d-%02d %02d:%02d:%02d", &year, &month, &day, &hour, &minute, &second);

    switch (rc) {
    case 6:
        NOW.sec = second;
    case 5:
        NOW.min = minute;
    case 4:
        NOW.hour = hour;
    case 3:
        NOW.day = day;
    case 2:
        NOW.month = month;
    case 1:
        NOW.year = year;
    }

    bool ok = rtc_set_datetime(&NOW);
    sleep_us(64);

    char s[64];
    snprintf(s, sizeof(s), "SYS    SET TIME %s", ok ? "OK" : "ERROR");
    tx(s);
}
