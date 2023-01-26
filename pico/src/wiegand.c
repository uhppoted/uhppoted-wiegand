#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/gpio.h"
#include "hardware/rtc.h"
#include "hardware/watchdog.h"

#include "pico/binary_info.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "pico/util/datetime.h"

#include "../include/acl.h"
#include "../include/buzzer.h"
#include "../include/cli.h"
#include "../include/controller.h"
#include "../include/emulator.h"
#include "../include/led.h"
#include "../include/sdcard.h"
#include "../include/sys.h"
#include "../include/wiegand.h"

#define VERSION "v0.0.0"

#define UART0 uart0
#define UART1 uart1
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE

// PIOs
const PIO PIO_READER = pio0;
const PIO PIO_BLINK = pio0;
const PIO PIO_WRITER = pio1;
const PIO PIO_LED = pio1;
const PIO PIO_BUZZER = pio1;

const uint SM_READER = 0;
const uint SM_BLINK = 1;
const uint SM_WRITER = 0;
const uint SM_LED = 1;
const uint SM_BUZZER = 2;

const enum pio_interrupt_source IRQ_READER = pis_sm0_rx_fifo_not_empty;
const enum pio_interrupt_source IRQ_LED = pis_sm1_rx_fifo_not_empty;

const uint PIO_READER_IRQ = PIO0_IRQ_0;
const uint PIO_LED_IRQ = PIO1_IRQ_0;

// GPIO
const uint GPIO_0 = 0;   // Pico 1
const uint GPIO_1 = 1;   // Pico 2
const uint GPIO_2 = 2;   // Pico 4
const uint GPIO_3 = 3;   // Pico 5
const uint GPIO_4 = 4;   // Pico 6
const uint GPIO_5 = 5;   // Pico 7
const uint GPIO_6 = 6;   // Pico 9
const uint GPIO_7 = 7;   // Pico 10
const uint GPIO_8 = 8;   // Pico 11
const uint GPIO_9 = 9;   // Pico 12
const uint GPIO_10 = 10; // Pico 14
const uint GPIO_11 = 11; // Pico 15
const uint GPIO_12 = 12; // Pico 16
const uint GPIO_13 = 13; // Pico 17
const uint GPIO_14 = 14; // Pico 19
const uint GPIO_15 = 15; // Pico 20
const uint GPIO_16 = 16; // Pico 21
const uint GPIO_17 = 17; // Pico 22
const uint GPIO_18 = 18; // Pico 24
const uint GPIO_19 = 19; // Pico 25
const uint GPIO_20 = 20; // Pico 26
const uint GPIO_21 = 21; // Pico 27
const uint GPIO_25 = 25; // Pico LED

const uint UART0_TX = GPIO_0; // Pico 1
const uint UART0_RX = GPIO_1; // Pico 2
const uint UART1_TX = GPIO_4; // Pico 6
const uint UART1_RX = GPIO_5; // Pico 7

const uint SD_CLK = GPIO_2;
const uint SD_SI = GPIO_3;
const uint SD_SO = GPIO_4;
const uint SD_CS = GPIO_5;
const uint SD_DET = GPIO_6;

const uint MODE_CONTROLLER = GPIO_7;
const uint MODE_EMULATOR = GPIO_8;

const uint LED_PIN = GPIO_25;
const uint BUZZER = GPIO_10;
const uint RED_LED = GPIO_12;
const uint YELLOW_LED = GPIO_13;
const uint ORANGE_LED = GPIO_14;
const uint GREEN_LED = GPIO_15;

const uint READER_D0 = GPIO_16;
const uint READER_D1 = GPIO_17;
const uint WRITER_D0 = GPIO_18;
const uint WRITER_D1 = GPIO_19;
const uint READER_LED = GPIO_20;
const uint WRITER_LED = GPIO_21;

const uint32_t MSG = 0xf0000000;
const uint32_t MSG_WATCHDOG = 0x00000000;
const uint32_t MSG_SYSCHECK = 0x10000000;
const uint32_t MSG_RX = 0x20000000;
const uint32_t MSG_TX = 0x30000000;
const uint32_t MSG_CARD_READ = 0x40000000;
const uint32_t MSG_LED = 0x50000000;
const uint32_t MSG_RXI = 0xd0000000;
const uint32_t MSG_SYSINIT = 0xe0000000;
const uint32_t MSG_DEBUG = 0xf0000000;

// FUNCTION PROTOTYPES

void setup_gpio(void);
void setup_uart(void);
void sysinit();

int64_t startup(alarm_id_t, void *);
int64_t off(alarm_id_t, void *);
int64_t timeout(alarm_id_t, void *);
bool watchdog(repeating_timer_t *);
bool syscheck(repeating_timer_t *);

// GLOBALS
enum MODE mode = UNKNOWN;
queue_t queue;

card last_card = {
    .facility_code = 0,
    .card_number = 0,
    .ok = false,
};

// UART
void on_uart0_rx() {
    char buffer[32];
    int ix = 0;

    while (uart_is_readable(UART0) && ix < sizeof(buffer)) {
        buffer[ix++] = uart_getc(UART0);
    }

    if (ix > 0) {
        char *b;

        if ((b = calloc(ix + 1, 1)) != NULL) {
            memmove(b, buffer, ix);
            uint32_t msg = MSG_RX | ((uint32_t)b & 0x0fffffff); // SRAM_BASE is 0x20000000
            if (queue_is_full(&queue) || !queue_try_add(&queue, &msg)) {
                free(b);
            }
        }

        ix = 0;
    }
}

const LED SYS_LED = {
    .pin = LED_PIN,
    .timer = -1,
};

const LED TIMEOUT_LED = {
    .pin = YELLOW_LED,
    .timer = -1,
};

const LED BAD_CARD = {
    .pin = ORANGE_LED,
    .timer = -1,
};

const LED GOOD_LED = {
    .pin = RED_LED,
    .timer = -1,
};

int main() {
    bi_decl(bi_program_description("Pico-Wiegand interface"));
    bi_decl(bi_program_version_string(VERSION));
    bi_decl(bi_1pin_with_name(LED_PIN, "on-board LED"));

    bi_decl(bi_1pin_with_name(READER_D0, "reader D0"));
    bi_decl(bi_1pin_with_name(READER_D1, "reader D1"));
    bi_decl(bi_1pin_with_name(READER_LED, "reader LED (out)"));

    bi_decl(bi_1pin_with_name(WRITER_D0, "writer D0"));
    bi_decl(bi_1pin_with_name(WRITER_D1, "writer D1"));
    bi_decl(bi_1pin_with_name(WRITER_LED, "writer LED (in)"));

    stdio_init_all();
    setup_gpio();
    watchdog_enable(5000, true);

    // ... initialise RTC
    rtc_init();
    sleep_us(64);

    // ... initialise FIFO, UART and timers
    queue_init(&queue, sizeof(uint32_t), 64);
    setup_uart();
    alarm_pool_init_default();

    // ... initialise reader/emulator
    add_alarm_in_ms(250, startup, NULL, true);
    clear_screen();

    while (true) {
        uint32_t v;
        queue_remove_blocking(&queue, &v);

        if ((v & MSG) == MSG_SYSINIT) {
            sysinit();
        }

        if ((v & MSG) == MSG_SYSCHECK) {
            sys_ok();
        }

        if ((v & MSG) == MSG_WATCHDOG) {
            watchdog_update();
            blink((LED *)&SYS_LED);
        }

        if ((v & MSG) == MSG_RX) {
            char *b = (char *)(SRAM_BASE | (v & 0x0fffffff));
            rx(b);
            free(b);
        }

        if ((v & MSG) == MSG_TX) {
            char *b = (char *)(SRAM_BASE | (v & 0x0fffffff));
            puts(b);
            free(b);
        }

        if ((v & MSG) == MSG_RXI) {
            on_card_rxi(v & 0x0fffffff);
        }

        if ((v & MSG) == MSG_CARD_READ) {
            char s[64];

            on_card_read(v & 0x0fffffff);
            cardf(&last_card, s, sizeof(s));
            puts(s);
            blink(last_card.ok ? (LED *)&GOOD_LED : (LED *)&BAD_CARD);
        }

        if ((v & MSG) == MSG_LED) {
            led_event(v & 0x0fffffff);
        }

        if ((v & MSG) == MSG_DEBUG) {
            char s[64];
            snprintf(s, sizeof(s), "DEBUG %d", v & 0x0fffffff);
            puts(s);
        }
    }

    // ... cleanup

    queue_free(&queue);
}

void setup_gpio() {
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(RED_LED);
    gpio_init(YELLOW_LED);
    gpio_init(ORANGE_LED);
    gpio_init(GREEN_LED);

    gpio_set_dir(RED_LED, GPIO_OUT);
    gpio_set_dir(YELLOW_LED, GPIO_OUT);
    gpio_set_dir(ORANGE_LED, GPIO_OUT);
    gpio_set_dir(GREEN_LED, GPIO_OUT);

    gpio_pull_up(RED_LED);
    gpio_pull_up(YELLOW_LED);
    gpio_pull_up(ORANGE_LED);
    gpio_pull_up(GREEN_LED);

    gpio_put(RED_LED, 1);
    gpio_put(YELLOW_LED, 1);
    gpio_put(ORANGE_LED, 1);
    gpio_put(GREEN_LED, 0);

    gpio_init(MODE_CONTROLLER);
    gpio_set_dir(MODE_CONTROLLER, GPIO_IN);
    gpio_pull_up(MODE_CONTROLLER);

    gpio_init(MODE_EMULATOR);
    gpio_set_dir(MODE_EMULATOR, GPIO_IN);
    gpio_pull_up(MODE_EMULATOR);

    gpio_init(SD_DET);
    gpio_set_dir(SD_DET, GPIO_IN);
    gpio_pull_down(SD_DET);
}

void setup_uart() {
    uart_init(UART0, 2400);
    gpio_set_function(UART0_TX, GPIO_FUNC_UART);
    gpio_set_function(UART0_RX, GPIO_FUNC_UART);
    uart_set_baudrate(UART0, BAUD_RATE);
    uart_set_hw_flow(UART0, false, false);
    uart_set_format(UART0, DATA_BITS, STOP_BITS, PARITY);
    uart_set_fifo_enabled(UART0, false);
    irq_set_exclusive_handler(UART0_IRQ, on_uart0_rx);
    irq_set_enabled(UART0_IRQ, true);
    uart_set_irq_enables(UART0, true, false);
}

void sysinit() {
    static bool initialised = false;
    static repeating_timer_t watchdog_rt;
    static repeating_timer_t syscheck_rt;

    if (!initialised) {
        puts("                     SYS  STARTUP");

        if (!gpio_get(MODE_CONTROLLER) && gpio_get(MODE_EMULATOR)) {
            mode = CONTROLLER;
        } else if (gpio_get(MODE_CONTROLLER) && !gpio_get(MODE_EMULATOR)) {
            mode = EMULATOR;
        } else {
            mode = UNKNOWN;
        }

        // FIXME
        mode = EMULATOR;

        acl_initialise();
        controller_initialise();
        emulator_initialise();
        led_initialise(mode);
        buzzer_initialise(mode);
        sdcard_initialise(mode);

        // ... setup sys stuff
        add_repeating_timer_ms(1250, watchdog, NULL, &watchdog_rt);
        add_repeating_timer_ms(5000, syscheck, NULL, &syscheck_rt);

        char dt[32];
        snprintf(dt, sizeof(dt), "%s %s", SYSDATE, SYSTIME);
        sys_settime(dt);

        rtc_get_datetime(&last_card.timestamp);
        sys_ok();
        set_scroll_area();

        initialised = true;
    }
}

bool watchdog(repeating_timer_t *rt) {
    uint32_t v = MSG_WATCHDOG | 0x0000000;

    if (!queue_is_full(&queue)) {
        queue_try_add(&queue, &v);
    }

    return true;
}

bool syscheck(repeating_timer_t *rt) {
    uint32_t v = MSG_SYSCHECK | 0x0000000;

    if (!queue_is_full(&queue)) {
        queue_try_add(&queue, &v);
    }

    return true;
}

void blink(LED *led) {
    if (led->timer > 0) {
        cancel_alarm(led->timer);
    }

    gpio_put(led->pin, 0);

    led->timer = add_alarm_in_ms(250, off, (void *)led, true);
}

int64_t startup(alarm_id_t id, void *data) {
    uint32_t v = MSG_SYSINIT | 0x0000000;

    if (!queue_is_full(&queue)) {
        queue_try_add(&queue, &v);
    }

    return 0;
}

int64_t off(alarm_id_t id, void *data) {
    const LED *led = data;

    gpio_put(led->pin, 1);

    return 0;
}

// Ref. https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
// Ref. https://stackoverflow.com/questions/109023/count-the-number-of-set-bits-in-a-32-bit-integer
int bits(uint32_t v) {
    v = v - ((v >> 1) & 0x55555555);
    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
    v = (v + (v >> 4)) & 0x0f0f0f0f;

    return (v * 0x01010101) >> 24;
}

void cardf(const card *c, char *s, int N) {
    char t[24];
    char n[16];
    char g[16];

    timef(c->timestamp, t, sizeof(t));

    if (c->card_number == 0) {
        snprintf(n, sizeof(n), "---");
        snprintf(g, sizeof(g), "");
    } else {
        snprintf(n, sizeof(n), "%u%05u", c->facility_code, c->card_number);

        if (!c->ok) {
            snprintf(g, sizeof(g), "INVALID");
        } else {
            switch (c->granted) {
            case GRANTED:
                snprintf(g, sizeof(g), "ACCESS GRANTED");
                break;

            case DENIED:
                snprintf(g, sizeof(g), "ACCESS DENIED");
                break;

            default:
                snprintf(g, sizeof(g), "-");
            }
        }
    }

    snprintf(s, N, "%-19s  CARD %-8s %s", t, n, g);
}

int timef(const datetime_t timestamp, char *s, int N) {
    if (is_valid(timestamp)) {
        snprintf(s, N, "%04d-%02d-%02d %02d:%02d:%02d",
                 timestamp.year,
                 timestamp.month,
                 timestamp.day,
                 timestamp.hour,
                 timestamp.min,
                 timestamp.sec);
    } else {
        snprintf(s, N, "---- -- -- --:--:--");
    }
}

bool is_valid(const datetime_t t) {
    if (t.year < 2000) {
        return false;
    }

    if (t.month < 1 || t.month > 12) {
        return false;
    }

    if (t.day < 1 || t.day > 31) {
        return false;
    }

    if (t.hour < 0 || t.hour > 23) {
        return false;
    }

    if (t.min < 0 || t.min > 59) {
        return false;
    }

    if (t.sec < 0 || t.sec > 59) {
        return false;
    }

    return true;
}
