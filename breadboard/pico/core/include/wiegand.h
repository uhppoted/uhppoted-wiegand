#pragma once

#include "hardware/pio.h"

#include "pico/util/queue.h"

#define CARD_PIN_SIZE 5
#define CARD_NAME_SIZE 48
#define MAX_CARDS 32

extern const uint UART0_RX;
extern const uint UART0_TX;
extern const uint ONBOARD_LED;

extern const PIO PIO_READER;
extern const PIO PIO_WRITER;

extern const uint SM_READER;
extern const uint SM_WRITER;

extern const uint PIO_READER_IRQ;
extern const enum pio_interrupt_source IRQ_READER;

enum MODE {
    UNKNOWN = 0,
    CONTROLLER = 1,
    EMULATOR = 2,
    INDETERMINATE = 0xff
};

enum ACCESS {
    GRANTED = 1,
    DENIED = 2,
    NEEDS_PIN = 3
};

typedef struct CARD {
    uint32_t card_number;
    datetime_t start;
    datetime_t end;
    bool allowed;
    char PIN[CARD_PIN_SIZE + 1];
    char name[CARD_NAME_SIZE + 1];
} CARD;

typedef struct card {
    datetime_t timestamp;
    uint32_t facility_code;
    uint32_t card_number;
    bool ok;
    enum ACCESS access;
} card;

extern enum MODE mode;

extern const uint JUMPER_READ;
extern const uint JUMPER_WRITE;

extern const uint READER_D0;
extern const uint READER_D1;
extern const uint READER_LED;
extern const uint WRITER_D0;
extern const uint WRITER_D1;
extern const uint WRITER_LED;
extern const uint RELAY_NO;
extern const uint RELAY_NC;
extern const uint PUSH_BUTTON;
extern const uint DOOR_SENSOR;
extern const uint BUZZER;

extern const uint SD_CS;
extern const uint SD_SI;
extern const uint SD_SO;
extern const uint SD_CLK;
extern const uint SD_DET;

extern const uint32_t MSG;
extern const uint32_t MSG_SYSINIT;
extern const uint32_t MSG_SYSCHECK;
extern const uint32_t MSG_WATCHDOG;
extern const uint32_t MSG_CARD;
extern const uint32_t MSG_CODE;
extern const uint32_t MSG_KEYPAD_DIGIT;
extern const uint32_t MSG_LED;
extern const uint32_t MSG_RELAY;
extern const uint32_t MSG_DOOR;
extern const uint32_t MSG_PUSHBUTTON;
extern const uint32_t MSG_RX;
extern const uint32_t MSG_TX;
extern const uint32_t MSG_LOG;
extern const uint32_t MSG_TCPD_POLL;
extern const uint32_t MSG_DEBUG;

extern enum MODE mode;
extern queue_t queue;
extern card last_card;
