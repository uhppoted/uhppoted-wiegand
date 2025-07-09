#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/bootrom.h"
#include "pico/stdlib.h"

#include <M5.h>
#include <SK6812.h>
#include <buffer.h>
#include <cli.h>
#include <log.h>
#include <sys.h>
#include <wiegand.h>

#define LOGTAG "CLI"

void cli_rxchar(uint8_t ch);
bool cli_on_terminal_report(const char *buffer, int N);
bool cli_up_arrow(const char *buffer, int N);

void echo(const char *line);
void clearline();
void cpr(char *cmd);
void display(const char *fmt, ...);
void exec(char *cmd);

void set_LED(const char *);
void blink_LED(const char *);
void swipe(char *);
void keypad(const char *);
void set_keypad_mode(const char *);
void reboot();
void bootsel();

void clear();
void help();
void debug(const char *);

extern const constants HW;

struct {
    int rows;
    int columns;
    bool escaped;

    buffer rx;

    struct {
        char bytes[64];
        int ix;
    } buffer;

    struct {
        char bytes[64];
        int ix;
    } last;

    struct {
        char bytes[64];
        int ix;
    } escape;
} cli = {
    .rows = 40,
    .columns = 120,
    .escaped = false,
    .rx = {
        .head = 0,
        .tail = 0,
    },
    .buffer = {
        .bytes = {0},
        .ix = 0,
    },
    .last = {
        .bytes = {0},
        .ix = 0,
    },
    .escape = {
        .bytes = {0},
        .ix = 0,
    },
};

extern const char *TERMINAL_CLEAR;
extern const char *TERMINAL_QUERY_CODE;
extern const char *TERMINAL_QUERY_STATUS;
extern const char *TERMINAL_QUERY_SIZE;
extern const char *TERMINAL_SET_SCROLL_AREA;
extern const char *TERMINAL_ECHO;
extern const char *TERMINAL_CLEARLINE;
extern const char *TERMINAL_DISPLAY;
extern const char *TERMINAL_AT;

const char CR = '\n';
const char LF = '\r';
const char ESC = 27;
const char BACKSPACE = 8;

const char *HELP[] = {
    "",
    "",
    "WIEGAND EMULATOR Rev.0",
    "",
    "Commands:",
    "  CARD nnnnnn dddddd    writes card + (optional) keycode to the",
    "                        Wiegand interface",
    "  CODE dddddd           writes keypad code to the Wiegand interface",
    "  SET LED <#RGB>        sets the indicator LED colour",
    "  BLINK <#RGB> <count>  blinks the indicator LED 'count' times",
    "  SET KEYPAD <4|8>      sets the keypad mode (4-bit or 8-bit)",
    "  REBOOT                reboots the board",
    "  BOOTSEL               reboots the board into BOOTSEL mode",
    "",
    "  CLEAR                 clears the screen",
    "  HELP                  displays CLI usage information",
    ""};

/** Unified stdin for CLI.
 *
 */
void cli_callback(void *data) {
    int count = 0;
    int ch;

    while ((ch = getchar_timeout_us(0)) != PICO_ERROR_TIMEOUT) {
        buffer_push(&cli.rx, ch);
        count++;
    }

    if (count > 0) {
        buffer *b = &cli.rx;

        push((message){
            .message = MSG_TTY,
            .tag = MESSAGE_BUFFER,
            .buffer = b,
        });
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

/** Processes received characters.
 *
 * Flushes buffer after processing any terminal ESC sequence as the simplest way
 * of handling ANSI escape sequences.
 */
void cli_rx(buffer *b) {
    buffer_flush(b, cli_rxchar);
}

void cli_rxchar(uint8_t ch) {
    // ... terminal ESC sequence ?
    if (cli.escaped) {
        if (ch == ESC) {
            cli_on_terminal_report(cli.escape.bytes, cli.escape.ix);

            cli.escaped = true;
            cli.escape.bytes[0] = ESC;
            cli.escape.ix = 1;
        } else if (cli.escape.ix < sizeof(cli.escape.bytes)) {
            cli.escape.bytes[cli.escape.ix++] = ch;

            if (cli_up_arrow(cli.escape.bytes, cli.escape.ix)) {
                cli.escape.ix = 0;
                cli.escaped = false;
            } else if (cli_on_terminal_report(cli.escape.bytes, cli.escape.ix)) {
                cli.escape.ix = 0;
                cli.escaped = false;
            }
        } else {
            cli.escape.ix = 0;
            cli.escaped = false;
        }

        return;
    }

    switch (ch) {
    case ESC:
        cli.escaped = true;
        cli.escape.bytes[0] = ESC;
        cli.escape.ix = 1;
        break;

    case BACKSPACE:
        if (cli.buffer.ix > 0) {
            cli.buffer.bytes[--cli.buffer.ix] = 0;
            echo(cli.buffer.bytes);
        }
        break;

    case CR:
    case LF:
        if (cli.buffer.ix > 0) {
            exec(cli.buffer.bytes);
        }

        memset(cli.buffer.bytes, 0, sizeof(cli.buffer.bytes));
        cli.buffer.ix = 0;
        echo(cli.buffer.bytes);
        break;

    default:
        if (cli.buffer.ix < (sizeof(cli.buffer.bytes) - 1)) {
            cli.buffer.bytes[cli.buffer.ix++] = ch;
            cli.buffer.bytes[cli.buffer.ix] = 0;

            echo(cli.buffer.bytes);
        }
    }
}

/** Processes ANSI/VT100 terminal report.
 *
 */
bool cli_on_terminal_report(const char *data, int N) {
    // ... report device code ?
    // e.g. [27 91 53 55 59 50 48 50 82 27 91 63 49 59 50 99 ]
    if (N >= 5 && data[0] == 27 && data[1] == '[' && data[N - 1] == 'c') {
        return true;
    }

    // ... report device status?
    if (N >= 4 && data[0] == 27 && data[1] == '[' && data[2] == '0' && data[3] == 'n') {
        return true;
    }

    // ... report cursor position (ESC[#;#R)
    // e.g. [27 91 53 55 59 50 48 50 82 ]
    if (N >= 6 && data[0] == 27 && data[1] == '[' && data[N - 1] == 'R') {
        char *code = strndup(&data[2], N - 3);

        cpr(code);
        free(code);
        return true;
    }

    return false;
}

/** 'up arrow' puts last command in terminal buffer.
 *
 */
bool cli_up_arrow(const char *data, int N) {
    if (N == 3 && data[0] == ESC && data[1] == 0x5b && data[2] == 0x41) {
        memset(cli.buffer.bytes, 0, sizeof(cli.buffer.bytes));
        memmove(cli.buffer.bytes, cli.last.bytes, cli.last.ix);
        cli.buffer.ix = cli.last.ix;

        echo(cli.buffer.bytes);
        return true;
    }

    return false;
}

/* Clears the terminal and queries window size
 *
 */
void clear() {
    print(TERMINAL_CLEAR);
    print(TERMINAL_QUERY_SIZE);
}

/* Cursor position report.
 *
 *  Sets the scrolling window with space at the bottom for the command echo.
 */
void cpr(char *cmd) {
    int rows;
    int cols;
    int rc = sscanf(cmd, "%d;%d", &rows, &cols);

    if (rc > 0) {
        cli.rows = rows;
        cli.columns = cols;

        if (rows > 20) {
            int h = rows - 5;
            char s[24];

            snprintf(s, sizeof(s), TERMINAL_SET_SCROLL_AREA, h);
            print(s);
        }
    }
}

/* Saves the cursor position, displays the current command buffer and then restores
 *  the cursor position
 *
 */
void echo(const char *cmd) {
    char s[64];
    int h = cli.rows - 4;

    snprintf(s, sizeof(s), TERMINAL_ECHO, h, cmd);
    print(s);
}

/* Saves the cursor position, clears the command line, redisplays the prompt and then
 *   restores the cursor position.
 *
 */
void clearline() {
    int h = cli.rows - 4;
    char s[24];

    snprintf(s, sizeof(s), TERMINAL_CLEARLINE, h);
    print(s);
}

/* Saves the cursor position, displays the command output and then restores the cursor
 *   position
 *
 */
void display(const char *fmt, ...) {
    int y = cli.rows - 3;
    char text[256];
    char s[256];

    va_list args;

    va_start(args, fmt);
    vsnprintf(text, sizeof(text), fmt, args);
    va_end(args);

    snprintf(s, sizeof(s), TERMINAL_DISPLAY, y, text);
    print(s);
}

void exec(char *cmd) {
    char s[128];

    memmove(cli.last.bytes, cli.buffer.bytes, cli.buffer.ix);
    cli.last.ix = cli.buffer.ix;

    if (strncasecmp(cmd, "card ", 5) == 0) {
        swipe(&cmd[5]);
    } else if (strncasecmp(cmd, "code ", 5) == 0) {
        keypad(&cmd[5]);
    } else if (strncasecmp(cmd, "set LED ", 8) == 0) {
        set_LED(&cmd[8]);
    } else if (strncasecmp(cmd, "blink ", 6) == 0) {
        blink_LED(&cmd[6]);
    } else if (strncasecmp(cmd, "set keypad ", 11) == 0) {
        set_keypad_mode(&cmd[11]);
    } else if (strncasecmp(cmd, "reboot", 6) == 0) {
        reboot();
    } else if (strncasecmp(cmd, "bootsel", 7) == 0) {
        bootsel();
    } else if (strncasecmp(cmd, "clear", 5) == 0) {
        clear();
    } else if (strncasecmp(cmd, "help", 4) == 0) {
        help();
    } else if (strncasecmp(cmd, "debug", 5) == 0) {
        debug(&cmd[5]);
    } else {
        display("unknown command (%s)\n", cmd);
    }
}

void debug(const char *cmd) {
    bool v = gpio_get(HW.LED);

    debugf(LOGTAG, ">>> LED %d", v);
}

/* Cancels watchdog updates.
 *
 */
void reboot() {
    display("... rebooting ... ");
    sys_reboot();
}

/* Reboots into USB BOOTSEL mode.
 *
 */
void bootsel() {
    display("... rebooting to BOOTSEL... ");
    reset_usb_boot(0, 0);
}

/* Card swipe + (optional) keycode emulation.
 *  Extract the facility code, card number and (optionally) the keycode from
 *  the command and writes the card and keycode (if present) to the Wiegand
 *  interface.
 */
void swipe(char *cmd) {
    uint32_t facility_code = FACILITY_CODE;
    uint32_t card = 0;
    char *token = strtok(cmd, " ,");
    char *code;

    if (token != NULL) {
        int N = strlen(token);

        if (N < 5) {
            if (sscanf(cmd, "%0u", &card) < 1) {
                return;
            }
        } else {
            if (sscanf(&token[N - 5], "%05u", &card) < 1) {
                return;
            }

            if (N == 6 && sscanf(token, "%01u", &facility_code) < 1) {
                return;
            } else if (N == 7 && sscanf(token, "%02u", &facility_code) < 1) {
                return;
            } else if (N == 8 && sscanf(token, "%03u", &facility_code) < 1) {
                return;
            } else if (N > 8) {
                return;
            }
        }

        if ((code = strtok(NULL, " ,")) == NULL) {
            if (!write_card(facility_code, card)) {
                debugf(LOGTAG, "card %u%05u error", facility_code, card);
                return;
            }

            debugf(LOGTAG, "card %u%05u write ok", facility_code, card);
        } else {
            if (!write_card(facility_code, card)) {
                debugf(LOGTAG, "card %u%05u error", facility_code, card);
                return;
            }

            int N = strlen(code);
            for (int i = 0; i < N; i++) {
                char key = cmd[i];

                if (!write_keycode(key)) {
                    debugf(LOGTAG, "keycode %c error", key);
                    return;
                }
            }

            debugf(LOGTAG, "card+ %u%05u + %u write ok", facility_code, card, code);
        }
    }
}

/* Keypad emulation.
 *  Sends the keycode code as 4-bit/8-bit burst mode Wiegand.
 *
 */
void keypad(const char *cmd) {
    int N = strlen(cmd);

    if (N > 0) {
        for (int i = 0; i < N; i++) {
            char key = cmd[i];

            if (!write_keycode(key)) {
                debugf(LOGTAG, "keycode %c error", key);
                return;
            }
        }

        debugf(LOGTAG, "keycode %s ok", cmd);
    }
}
/* Sets the keypad mode (4 bit or 8 bit).
 *
 */
void set_keypad_mode(const char *cmd) {
    uint32_t v;
    int rc;

    if ((rc = sscanf(cmd, "%u", &v)) == 1) {
        if (!keypad_mode(v)) {
            debugf(LOGTAG, "invalid keypad mode (%lu)", v);
        }
    }
}

/* Sets the front panel LED.
 *
 */
void set_LED(const char *cmd) {
    uint32_t v;
    int rc;

    if ((rc = sscanf(cmd, "#%x", &v)) == 1) {
        uint8_t r = (v >> 16) & 0x00ff;
        uint8_t g = (v >> 8) & 0x00ff;
        uint8_t b = (v >> 0) & 0x00ff;

        SK6812_set(r, g, b);

        debugf(LOGTAG, "set LED rgb:(%u,%u,%u)", r, g, b);
    }
}

/* Blinks the front panel LED.
 *
 */
void blink_LED(const char *cmd) {
    uint32_t v;
    int rc;
    uint N = 1;

    if ((rc = sscanf(cmd, "#%x %u", &v, &N)) > 0) {
        uint8_t r = (v >> 16) & 0x00ff;
        uint8_t g = (v >> 8) & 0x00ff;
        uint8_t b = (v >> 0) & 0x00ff;

        SK6812_blink(r, g, b, N);

        debugf(LOGTAG, "blink LED rgb:(%u,%u,%u) count:%u", r, g, b, N);
    }
}

void help() {
    int N = sizeof(HELP) / sizeof(char *);
    int X = cli.columns - 80;
    int Y = 0;
    char s[128];

    for (int i = 0; i < N; i++) {
        snprintf(s, sizeof(s), TERMINAL_AT, Y + i, X, HELP[i]);
        print(s);
    }
}
