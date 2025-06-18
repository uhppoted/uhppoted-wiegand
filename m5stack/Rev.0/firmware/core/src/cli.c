// #include <math.h>
// #include <stdarg.h>
// #include <stdbool.h>
#include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

// #include "pico/bootrom.h"
#include "pico/stdlib.h"
// #include "pico/util/datetime.h"

// #include <breakout.h>
#include <cli.h>
// #include <log.h>
#include <sys.h>
// #include <types/buffer.h>

#define LOGTAG "CLI"

// const uint32_t CLI_TIMEOUT = 5000; // ms
// const uint8_t height = 25;

// void cli_rxchar(uint8_t ch);
// bool cli_on_terminal_report(const char *buffer, int N);
// bool cli_up_arrow(const char *buffer, int N);

// void echo(const char *line);
// void clearline();
// void cpr(char *cmd);
// void display(const char *fmt, ...);
// void exec(char *cmd);

// void show_state();
// void clear_errors();
// void trace(const char *interval);
// void reboot(bool);

// void clear();
// void help();
// void debug();

struct {
    //     int rows;
    //     int columns;
    //     bool escaped;

    //     circular_buffer rx;

    //     struct {
    //         char bytes[64];
    //         int ix;
    //     } buffer;

    //     struct {
    //         char bytes[64];
    //         int ix;
    //     } last;

    //     struct {
    //         char bytes[64];
    //         int ix;
    //     } escape;
} cli = {
    //     .rows = 40,
    //     .columns = 120,
    //     .escaped = false,
    //     .rx = {
    //         .head = 0,
    //         .tail = 0,
    //     },
    //     .buffer = {
    //         .bytes = {0},
    //         .ix = 0,
    //     },
    //     .last = {
    //         .bytes = {0},
    //         .ix = 0,
    //     },
    //     .escape = {
    //         .bytes = {0},
    //         .ix = 0,
    //     },
};

extern const char *TERMINAL_CLEAR;
extern const char *TERMINAL_QUERY_CODE;
// extern const char *TERMINAL_QUERY_STATUS;
extern const char *TERMINAL_QUERY_SIZE;
// extern const char *TERMINAL_SET_SCROLL_AREA;
// extern const char *TERMINAL_ECHO;
// extern const char *TERMINAL_CLEARLINE;
// extern const char *TERMINAL_DISPLAY;
// extern const char *TERMINAL_AT;

// const char CR = '\n';
// const char LF = '\r';
// const char ESC = 27;
// const char BACKSPACE = 8;

// const char *HELP[] = {
//     "",
//     "",
//     "WIEGAND EMULATOR Rev.0",
//     "",
//     "Commands:",
//     "  status",
//     "  clear errors",
//     "  scan",
//     "  trace <off|on|[0-300]>",
//     "  reboot",
//     "  bootsel",
//     "",
//     "  clear",
//     "  help",
//     ""};

/** Unified stdin for CLI.
 *
 */
void cli_callback(void *data) {
    int count = 0;
    int ch;

    while ((ch = getchar_timeout_us(0)) != PICO_ERROR_TIMEOUT) {
        //     buffer_push(&cli.rx, ch);
        count++;
    }

    if (count > 0) {
        printf(">>>>>>> WOOT %d\n", count);
        //     circular_buffer *b = &cli.rx;

        //     push((message) {
        //         .message = MSG_TTY,
        //         .tag = MESSAGE_BUFFER,
        //         .buffer = b,
        //     });
    }
}

/** Sets the scroll area.
 *
 */
void cli_init() {
    stdio_set_chars_available_callback(cli_callback, &cli);

    print(TERMINAL_CLEAR);
    print(TERMINAL_QUERY_CODE);
    print(TERMINAL_QUERY_SIZE);
}

// /** Processes received characters.
//  *
//  * Flushes buffer after processing any terminal ESC sequence as the simplest way
//  * of handling ANSI escape sequences.
//  */
// void cli_rx(circular_buffer *buffer) {
//     buffer_flush(buffer, cli_rxchar);
// }

// void cli_rxchar(uint8_t ch) {
//     // ... terminal ESC sequence ?
//     if (cli.escaped) {
//         if (ch == ESC) {
//             cli_on_terminal_report(cli.escape.bytes, cli.escape.ix);

//             cli.escaped = true;
//             cli.escape.bytes[0] = ESC;
//             cli.escape.ix = 1;
//         } else if (cli.escape.ix < sizeof(cli.escape.bytes)) {
//             cli.escape.bytes[cli.escape.ix++] = ch;

//             if (cli_up_arrow(cli.escape.bytes, cli.escape.ix)) {
//                 cli.escape.ix = 0;
//                 cli.escaped = false;
//             } else if (cli_on_terminal_report(cli.escape.bytes, cli.escape.ix)) {
//                 cli.escape.ix = 0;
//                 cli.escaped = false;
//             }
//         } else {
//             cli.escape.ix = 0;
//             cli.escaped = false;
//         }

//         return;
//     }

//     switch (ch) {
//     case ESC:
//         cli.escaped = true;
//         cli.escape.bytes[0] = ESC;
//         cli.escape.ix = 1;
//         break;

//     case BACKSPACE:
//         if (cli.buffer.ix > 0) {
//             cli.buffer.bytes[--cli.buffer.ix] = 0;
//             echo(cli.buffer.bytes);
//         }
//         break;

//     case CR:
//     case LF:
//         if (cli.buffer.ix > 0) {
//             exec(cli.buffer.bytes);
//         }

//         memset(cli.buffer.bytes, 0, sizeof(cli.buffer.bytes));
//         cli.buffer.ix = 0;
//         echo(cli.buffer.bytes);
//         break;

//     default:
//         if (cli.buffer.ix < (sizeof(cli.buffer.bytes) - 1)) {
//             cli.buffer.bytes[cli.buffer.ix++] = ch;
//             cli.buffer.bytes[cli.buffer.ix] = 0;

//             echo(cli.buffer.bytes);
//         }
//     }
// }

// /** Processes ANSI/VT100 terminal report.
//  *
//  */
// bool cli_on_terminal_report(const char *data, int N) {
//     // ... report device code ?
//     // e.g. [27 91 53 55 59 50 48 50 82 27 91 63 49 59 50 99 ]
//     if (N >= 5 && data[0] == 27 && data[1] == '[' && data[N - 1] == 'c') {
//         return true;
//     }

//     // ... report device status?
//     if (N >= 4 && data[0] == 27 && data[1] == '[' && data[2] == '0' && data[3] == 'n') {
//         set_mode(MODE_CLI);
//         return true;
//     }

//     // ... report cursor position (ESC[#;#R)
//     // e.g. [27 91 53 55 59 50 48 50 82 ]
//     if (N >= 6 && data[0] == 27 && data[1] == '[' && data[N - 1] == 'R') {
//         char *code = strndup(&data[2], N - 3);

//         cpr(code);
//         free(code);
//         return true;
//     }

//     return false;
// }

// /** 'up arrow' puts last command in terminal buffer.
//  *
//  */
// bool cli_up_arrow(const char *data, int N) {
//     if (N == 3 && data[0] == ESC && data[1] == 0x5b && data[2] == 0x41) {
//         memset(cli.buffer.bytes, 0, sizeof(cli.buffer.bytes));
//         memmove(cli.buffer.bytes, cli.last.bytes, cli.last.ix);
//         cli.buffer.ix = cli.last.ix;

//         echo(cli.buffer.bytes);
//         return true;
//     }

//     return false;
// }

// /* Clears the terminal and queries window size
//  *
//  */
// void clear() {
//     print(TERMINAL_CLEAR);
//     print(TERMINAL_QUERY_SIZE);
// }

// /* Cursor position report.
//  *
//  *  Sets the scrolling window with space at the bottom for the command echo.
//  */
// void cpr(char *cmd) {
//     int rows;
//     int cols;
//     int rc = sscanf(cmd, "%d;%d", &rows, &cols);

//     if (rc > 0) {
//         cli.rows = rows;
//         cli.columns = cols;

//         if (rows > 20) {
//             int h = rows - 5;
//             char s[24];

//             snprintf(s, sizeof(s), TERMINAL_SET_SCROLL_AREA, h);
//             print(s);
//         }
//     }
// }

// /* Saves the cursor position, displays the current command buffer and then restores
//  *  the cursor position
//  *
//  */
// void echo(const char *cmd) {
//     char s[64];
//     int h = cli.rows - 4;

//     snprintf(s, sizeof(s), TERMINAL_ECHO, h, cmd);
//     print(s);
// }

// /* Saves the cursor position, clears the command line, redisplays the prompt and then
//  *   restores the cursor position.
//  *
//  */
// void clearline() {
//     int h = cli.rows - 4;
//     char s[24];

//     snprintf(s, sizeof(s), TERMINAL_CLEARLINE, h);
//     print(s);
// }

// /* Saves the cursor position, displays the command output and then restores the cursor
//  *   position
//  *
//  */
// void display(const char *fmt, ...) {
//     int y = cli.rows - 3;
//     char text[256];
//     char s[256];

//     va_list args;

//     va_start(args, fmt);
//     vsnprintf(text, sizeof(text), fmt, args);
//     va_end(args);

//     snprintf(s, sizeof(s), TERMINAL_DISPLAY, y, text);
//     print(s);
// }

// void exec(char *cmd) {
//     char s[128];

//     memmove(cli.last.bytes, cli.buffer.bytes, cli.buffer.ix);
//     cli.last.ix = cli.buffer.ix;

//     if (strncasecmp(cmd, "get id", 6) == 0) {
//         get_ID();
//     } else if (strncasecmp(cmd, "get datetime", 12) == 0) {
//         get_datetime();
//     } else if (strncasecmp(cmd, "set datetime ", 13) == 0) {
//         set_datetime(&cmd[13]);
//     } else if (strncasecmp(cmd, "get date", 8) == 0) {
//         get_date();
//     } else if (strncasecmp(cmd, "set date ", 9) == 0) {
//         set_date(&cmd[9]);
//     } else if (strncasecmp(cmd, "get time", 8) == 0) {
//         get_time();
//     } else if (strncasecmp(cmd, "set time ", 9) == 0) {
//         set_time(&cmd[9]);
//     } else if (strncasecmp(cmd, "get weekday", 11) == 0) {
//         get_weekday();
//     } else if (strncasecmp(cmd, "unlock door ", 12) == 0) {
//         unlock_door(&cmd[12]);
//     } else if (strncasecmp(cmd, "lock door ", 10) == 0) {
//         lock_door(&cmd[10]);
//     } else if (strncasecmp(cmd, "set LED ", 8) == 0) {
//         set_LED(&cmd[8], true);
//     } else if (strncasecmp(cmd, "clear LED ", 10) == 0) {
//         set_LED(&cmd[10], false);
//     } else if (strncasecmp(cmd, "blink LED ", 10) == 0) {
//         blink_LED(&cmd[10]);
//     } else if (strncasecmp(cmd, "get doors", 9) == 0) {
//         get_doors();
//     } else if (strncasecmp(cmd, "get buttons", 11) == 0) {
//         get_buttons();
//     } else if (strncasecmp(cmd, "show state", 10) == 0) {
//         show_state();
//     } else if (strncasecmp(cmd, "clear errors", 12) == 0) {
//         clear_errors();
//     } else if (strncasecmp(cmd, "scan", 4) == 0) {
//         scan();
//     } else if (strncasecmp(cmd, "trace ", 6) == 0) {
//         trace(&cmd[6]);
//     } else if (strncasecmp(cmd, "reboot", 6) == 0) {
//         reboot(false);
//     } else if (strncasecmp(cmd, "bootsel", 7) == 0) {
//         reboot(true);
//     } else if (strncasecmp(cmd, "clear", 5) == 0) {
//         clear();
//     } else if (strncasecmp(cmd, "help", 4) == 0) {
//         help();
//     } else if (strncasecmp(cmd, "debug", 5) == 0) {
//         debug();
//     } else {
//         display("unknown command (%s)\n", cmd);
//     }
// }

// void debug() {
//     syserr_set(ERR_DEBUG, LOGTAG, "<<< DEBUG");
// }

// void show_state() {
//     infof(LOGTAG, ">>> restart  %s", syserr_get(ERR_RESTART) ? "error" : "ok");
//     infof(LOGTAG, ">>> I2C      %s", syserr_get(ERR_I2C) ? "error" : "ok");
//     infof(LOGTAG, ">>> queue    %s", syserr_get(ERR_QUEUE) ? "error" : "ok");
//     infof(LOGTAG, ">>> memory   %s", syserr_get(ERR_MEMORY) ? "error" : "ok");
//     infof(LOGTAG, ">>> watchdog %s %s", syserr_get(ERR_WATCHDOG) ? "error" : "ok", strcmp(WATCHDOG, "disabled") == 0 ? "disabled" : "enabled");
//     infof(LOGTAG, ">>> RTC      %s", syserr_get(ERR_RX8900SA) ? "error" : "ok");
//     infof(LOGTAG, ">>> U2       %s", syserr_get(ERR_U2) ? "error" : "ok");
//     infof(LOGTAG, ">>> U3       %s", syserr_get(ERR_U3) ? "error" : "ok");
//     infof(LOGTAG, ">>> U4       %s", syserr_get(ERR_U4) ? "error" : "ok");
//     infof(LOGTAG, ">>> debug    %s", syserr_get(ERR_DEBUG) ? "error" : "ok");
//     infof(LOGTAG, ">>> other    %s", syserr_get(ERR_UNKNOWN) ? "error" : "ok");
// }

// void clear_errors() {
//     err errors[] = {
//         ERR_I2C,
//         ERR_QUEUE,
//         ERR_MEMORY,
//         ERR_RX8900SA,
//         ERR_U2,
//         ERR_U3,
//         ERR_U4,
//         ERR_WATCHDOG,
//         ERR_RESTART,
//         ERR_DEBUG,
//         ERR_UNKNOWN,
//     };

//     int N = sizeof(errors) / sizeof(err);

//     for (int i = 0; i < N; i++) {
//         syserr_clear(errors[i]);
//     }

//     display("clear errors: ok\n");
// }

// void get_ID() {
//     char ID[32] = {0};
//     int N = sizeof(ID);

//     sys_id(ID, N);

//     display("get-ID: %s\n", ID);
// }

// void get_datetime() {
//     uint16_t year;
//     uint8_t month;
//     uint8_t day;
//     uint8_t hour;
//     uint8_t minute;
//     uint8_t second;
//     char datetime[20] = {0};

//     if (RTC_get_datetime(&year, &month, &day, &hour, &minute, &second, NULL)) {
//         snprintf(datetime, sizeof(datetime), "%04u-%02u-%02u %02u:%02u:%02u", year, month, day, hour, minute, second);
//     } else {
//         snprintf(datetime, sizeof(datetime), "---- -- --");
//     }

//     display("get-datetime: %s\n", datetime);
// }

// void set_datetime(const char *cmd) {
//     int year;
//     int month;
//     int day;
//     int hour;
//     int minute;
//     int second;
//     int rc;

//     if ((rc = sscanf(cmd, "%04d-%02d-%02d %02d:%02d:%02d", &year, &month, &day, &hour, &minute, &second)) == 6) {
//         RTC_set_datetime(year, month, day, hour, minute, second);
//         display("set-datetime: ok");
//         return;
//     }

//     if ((rc = sscanf(cmd, "%04d-%02d-%02d %02d:%02d", &year, &month, &day, &hour, &minute)) == 5) {
//         RTC_set_datetime(year, month, day, hour, minute, 0);
//         display("set-datetime: ok");
//         return;
//     }

//     display("*** INVALID DATE/TIME (%s)", cmd);
// }

// void get_date() {
//     uint16_t year;
//     uint8_t month;
//     uint8_t day;
//     char date[11] = {0};

//     if (RTC_get_datetime(&year, &month, &day, NULL, NULL, NULL, NULL)) {
//         snprintf(date, sizeof(date), "%04u-%02u-%02u", year, month, day);
//     } else {
//         snprintf(date, sizeof(date), "---- -- --");
//     }

//     display("get-date: %s", date);
// }

// void set_date(const char *cmd) {
//     int year;
//     int month;
//     int day;
//     int rc;

//     if ((rc = sscanf(cmd, "%04d-%02d-%02d", &year, &month, &day)) == 3) {
//         RTC_set_date(year, month, day);
//         display("set-date: ok");
//         return;
//     }

//     display("*** INVALID DATE (%s)", cmd);
// }

// void get_time() {
//     uint8_t hour;
//     uint8_t minute;
//     uint8_t second;
//     char time[10] = {0};

//     if (RTC_get_datetime(NULL, NULL, NULL, &hour, &minute, &second, NULL)) {
//         snprintf(time, sizeof(time), "%02u:%02u:%02u", hour, minute, second);
//     } else {
//         snprintf(time, sizeof(time), "--:--:--");
//     }

//     display("get-time: %s", time);
// }

// void set_time(const char *cmd) {
//     int hour;
//     int minute;
//     int second;
//     int rc;

//     if ((rc = sscanf(cmd, "%02d:%02d:%02d", &hour, &minute, &second)) == 3) {
//         RTC_set_time(hour, minute, second);
//         display("set-time: ok");
//         return;
//     }

//     if ((rc = sscanf(cmd, "%02d:%02d", &hour, &minute)) == 2) {
//         RTC_set_time(hour, minute, 0);
//         display("set-time: ok");
//         return;
//     }

//     display("*** INVALID TIME (%s)", cmd);
// }

// void get_weekday() {
//     uint8_t dow;
//     char weekday[10] = {0};

//     if (RTC_get_datetime(NULL, NULL, NULL, NULL, NULL, NULL, &dow)) {
//         if (dow == SUNDAY) {
//             snprintf(weekday, sizeof(weekday), "Sunday");
//         } else if (dow == MONDAY) {
//             snprintf(weekday, sizeof(weekday), "Monday");
//         } else if (dow == TUESDAY) {
//             snprintf(weekday, sizeof(weekday), "Tuesday");
//         } else if (dow == WEDNESDAY) {
//             snprintf(weekday, sizeof(weekday), "Wednesday");
//         } else if (dow == THURSDAY) {
//             snprintf(weekday, sizeof(weekday), "Thursday");
//         } else if (dow == FRIDAY) {
//             snprintf(weekday, sizeof(weekday), "Friday");
//         } else if (dow == SATURDAY) {
//             snprintf(weekday, sizeof(weekday), "Saturday");
//         } else {
//             snprintf(weekday, sizeof(weekday), "---");
//         }
//     } else {
//         snprintf(weekday, sizeof(weekday), "---");
//     }

//     display("get-weekday: %s", weekday);
// }

// void unlock_door(const char *cmd) {
//     int N = strlen(cmd);

//     if (N > 0) {
//         uint door;
//         int rc;

//         if ((rc = sscanf(cmd, "%u", &door)) == 1) {
//             if (doors_unlock(door)) {
//                 display("door %u unlocked", door);
//             }
//         }
//     }
// }

// void lock_door(const char *cmd) {
//     int N = strlen(cmd);

//     if (N > 0) {
//         uint relay;
//         int rc;

//         if ((rc = sscanf(cmd, "%u", &relay)) == 1) {
//             U4_clear_relay(relay);
//             display("door %u locked", relay);
//         }
//     }
// }

// void set_LED(const char *cmd, bool state) {
//     int N = strlen(cmd);

//     if (N > 0) {
//         uint LED;
//         int rc;

//         if ((rc = sscanf(cmd, "%u", &LED)) == 1) {
//             if (state) {
//                 U4_set_LED(LED);
//                 display("LED %u on", LED);

//             } else {
//                 U4_clear_LED(LED);
//                 display("LED %u off", LED);
//             }
//             return;
//         }

//         if (strncasecmp(cmd, "ERR", 3) == 0) {
//             if (state) {
//                 U4_set_ERR();
//                 display("ERR LED on");
//             } else {
//                 U4_clear_ERR();
//                 display("ERR LED off");
//             }
//             return;
//         }

//         if (strncasecmp(cmd, "IN", 2) == 0) {
//             if (state) {
//                 U4_set_IN();
//                 display("IN LED on");
//             } else {
//                 U4_clear_IN();
//                 display("IN LED off");
//             }
//             return;
//         }

//         if (strncasecmp(cmd, "SYS", 3) == 0) {
//             if (state) {
//                 U4_set_SYS();
//                 display("SYS LED on");
//             } else {
//                 U4_clear_SYS();
//                 display("SYS LED off");
//             }
//             return;
//         }
//     }
// }

// void blink_LED(const char *cmd) {
//     int N = strlen(cmd);

//     if (N > 0) {
//         uint LED;
//         int rc;

//         if ((rc = sscanf(cmd, "%u", &LED)) == 1) {
//             U4_blink_LED(LED, 5, 500);
//             display("blinking LED %u", LED);
//             return;
//         }

//         if (strncasecmp(cmd, "ERR", 3) == 0) {
//             U4_blink_ERR(5, 500);
//             display("blinking ERR LED");
//             return;
//         }

//         if (strncasecmp(cmd, "IN", 2) == 0) {
//             U4_blink_IN(5, 500);
//             display("blinking IN LED");
//             return;
//         }

//         if (strncasecmp(cmd, "SYS", 3) == 0) {
//             U4_blink_SYS(5, 500);
//             display("blinking SYS LED");
//             return;
//         }
//     }
// }

// void get_doors() {
//     char s[64];
//     int ix = 0;

//     ix += snprintf(&s[ix], sizeof(s) - ix, "doors ");
//     for (uint8_t door = 1; door <= 4; door++) {
//         bool open = U3_get_door(door);

//         ix += snprintf(&s[ix], sizeof(s) - ix, " %u:%s", door, open ? "open" : "closed");
//     };

//     display("%s", s);
// }

// void get_buttons() {
//     char s[64];
//     int ix = 0;

//     ix += snprintf(&s[ix], sizeof(s) - ix, "buttons ");
//     for (uint8_t door = 1; door <= 4; door++) {
//         bool pressed = U3_get_button(door);

//         ix += snprintf(&s[ix], sizeof(s) - ix, " %u:%s", door, pressed ? "pressed" : "released");
//     };

//     display("%s", s);
// }

// void scan() {
//     I2C0_scan();
//     I2C1_scan();
// }

// /* Sets the trace interval:
//  * - off:      trace disabled
//  * - on:       trace interval set to the compile time TRACE value
//  * - [0..300]: trace interval in seconds (float). Rounded to the nearest 0.25.
//  */
// void trace(const char *arg) {
//     int rc;
//     float interval;

//     if (strcasecmp(arg, "off") == 0) {
//         set_trace(0.0);
//         display("trace off");
//     } else if (strcasecmp(arg, "on") == 0) {
//         set_trace(TRACE);
//         display("trace %.2fs", roundf(4.0 * (float)TRACE) / 4.0);
//     } else if (((rc = sscanf(arg, "%f", &interval)) == 1) && (interval >= 0.0)) {
//         if (interval > 0.0) {
//             interval = roundf(4.0 * interval) / 4.0;
//             interval = interval < 0.25 ? 0.25 : interval;
//             interval = interval > 300. ? 300.0 : interval;
//             set_trace(interval);
//             display("trace %.2fs", interval);
//         } else {
//             set_trace(0.0);
//             display("trace off");
//         }
//     }

//     if (strcasecmp(arg, "RTC on") == 0) {
//         set_trace_RTC(true);
//         display("RTC trace on");
//     } else if (strcasecmp(arg, "RTC off") == 0) {
//         set_trace_RTC(false);
//         display("RTC trace off");
//     }
// }

// /* Tight loop until watchdog reboots the system.
//  *
//  */
// void reboot(bool bootsel) {
//     if (bootsel) {
//         display("... rebooting to BOOTSEL... ");
//         reset_usb_boot(0, 0);
//     } else {
//         display("... rebooting ... ");
//         sys_reboot();
//     }
// }

// void help() {
//     int N = sizeof(HELP) / sizeof(char *);
//     int X = cli.columns - 48;
//     int Y = 0;
//     char s[128];

//     for (int i = 0; i < N; i++) {
//         snprintf(s, sizeof(s), TERMINAL_AT, Y + i, X, HELP[i]);
//         print(s);
//     }
// }
