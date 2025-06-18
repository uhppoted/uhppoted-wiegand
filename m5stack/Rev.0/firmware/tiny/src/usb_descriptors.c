#include <bsp/board_api.h>
#include <tusb.h>

#define _PID_MAP(itf, n) ((CFG_TUD_##itf) << (n))

#define USB_VID 0xcafe
// #define USB_PID (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(VENDOR, 4))
#define USB_PID (0x4000 | _PID_MAP(CDC, 0))
#define USB_BCD 0x0200

uint8_t const *tud_descriptor_device_qualifier_cb(void);
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid);

enum {
    ITF_NUM_CDC_0 = 0,
    ITF_NUM_CDC_0_DATA,
    ITF_NUM_CDC_1,
    ITF_NUM_CDC_1_DATA,
    ITF_NUM_TOTAL
};

enum {
    STRID_LANGID = 0,   // 0: supported language ID
    STRID_MANUFACTURER, // 1: manufacturer
    STRID_PRODUCT,      // 2: product
    STRID_SERIAL,       // 3: serial number
    STRID_CDC_0,        // 4: CDC 0 interface
    STRID_CDC_1,        // 5: CDC 1 interface
    STRID_RESET,        // 6: reset interface
};

char const *string_desc_arr[] = {
    (const char[]){0x09, 0x04}, // 0: supported language is English (0x0409)
    "uhppoted",                 // 1: manufacturer
    "breakout",                 // 2: product
    NULL,                       // 3: serial number (null, uses unique ID)
    "log"                       // 4: CDC 0 interface
    "SSMP",                     // 5: CDC 1 interface
    "reset",                    // 6: reset interface
};

// USB device descriptor
tusb_desc_device_t const desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = USB_BCD, // USB 2.0

    .bDeviceClass = TUSB_CLASS_MISC,           // CDC is a subclass of misc
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,   // CDC uses common subclass
    .bDeviceProtocol = MISC_PROTOCOL_IAD,      // CDC uses IAD
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE, // 64 bytes

    .idVendor = USB_VID,
    .idProduct = USB_PID,
    .bcdDevice = 0x0100, // Device release number

    .iManufacturer = STRID_MANUFACTURER, // manufacturer string
    .iProduct = STRID_PRODUCT,           // product string
    .iSerialNumber = STRID_SERIAL,       // serial number string

    .bNumConfigurations = 0x01 // 1 configuration
};

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + CFG_TUD_CDC * TUD_CDC_DESC_LEN)

// CDC 0 interface notification, OUT and IN endpoints
#define EPNUM_CDC_0_NOTIFY (0x81)
#define EPNUM_CDC_0_OUT (0x02)
#define EPNUM_CDC_0_IN (0x82)

// CDC 1 interface notification, OUT and IN endpoints
#define EPNUM_CDC_1_NOTIFY (0x84)
#define EPNUM_CDC_1_OUT (0x05)
#define EPNUM_CDC_1_IN (0x85)

// configuration descriptors
uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x80, 100),
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_0, 4, EPNUM_CDC_0_NOTIFY, 8, EPNUM_CDC_0_OUT, EPNUM_CDC_0_IN, 64),
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_1, 4, EPNUM_CDC_1_NOTIFY, 8, EPNUM_CDC_1_OUT, EPNUM_CDC_1_IN, 64),
};

// device descriptor qualifier
tusb_desc_device_qualifier_t const desc_device_qualifier = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = USB_BCD,

    .bDeviceClass = TUSB_CLASS_CDC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,

    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .bNumConfigurations = 0x01,
    .bReserved = 0x00,
};

uint8_t const *tud_descriptor_device_cb(void) {
    return (uint8_t const *)&desc_device;
}

uint8_t const *tud_descriptor_device_qualifier_cb(void) {
    return (uint8_t const *)&desc_device_qualifier;
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;

    return desc_configuration;
}

static uint16_t _desc_str[32 + 1];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    size_t char_count;

    switch (index) {
    case STRID_LANGID:
        memcpy(&_desc_str[1], string_desc_arr[STRID_LANGID], 2);
        char_count = 1;
        break;

    case STRID_SERIAL:
        char_count = board_usb_get_serial(_desc_str + 1, 32);
        break;

    default:
        if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0]))) {
            return NULL;
        }

        const char *str = string_desc_arr[index];
        char_count = strlen(str);
        size_t const max_count = sizeof(_desc_str) / sizeof(_desc_str[0]) - 1; // -1 for string type

        if (char_count > max_count) {
            char_count = max_count;
        }

        // convert to UTF-16
        for (size_t i = 0; i < char_count; i++) {
            _desc_str[1 + i] = str[i];
        }
        break;
    }

    // first byte is the length (including header), second byte is string type
    _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (char_count * 2 + 2));

    return _desc_str;
}

// // FIXME move to usb.c ?
// bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) {
//     printf(">>> tud_vendor_control_xfer_cb %02x %02x\n", request->bmRequestType, request->bRequest);
//
//     if (stage != CONTROL_STAGE_SETUP) {
//         return true;
//     }
//
//     // // BOOTSEL reboot command (0x92) ?
//     // if ((request->bmRequestType == 0x40) && (request->bRequest == 0x92)) {
//     //     watchdog_reboot(0, 0, 0);
//     //     *((uint32_t *)0x20041FFC) = 0x64738219;  // Magic value for BOOTSEL mode
//     //     scb_hw->aircr = (0x5FA << 16) | (1 << 2);
//     //     while (1);
//     // }
//
//     return false;
// }
