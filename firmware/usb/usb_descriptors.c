#include "tusb.h"
#include <string.h>
#include "pico/unique_id.h"

/* Shared private-test IDs, not universally unique.
   Obtain assigned IDs before distributing hardware. */
#define THERMO_USB_VID 0x1209
#define THERMO_USB_PID 0x0001

/* Endpoint addresses: bit 7 specifies direction relative to the host. */
#define THERMO_EP_OUT 0x01
#define THERMO_EP_IN  0x81

#define THERMO_CONFIG_LENGTH \
    (TUD_CONFIG_DESC_LEN + TUD_VENDOR_DESC_LEN)

/* Describe the whole device; its interface declares the vendor class. */
static const tusb_desc_device_t device_descriptor = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0210,

    .bDeviceClass = 0,
    .bDeviceSubClass = 0,
    .bDeviceProtocol = 0,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor = THERMO_USB_VID,
    .idProduct = THERMO_USB_PID,
    /* New descriptor revision so Windows can reevaluate OS descriptors. */
    .bcdDevice = 0x0101,

    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1
};

/* One bus-powered configuration, one vendor interface, two bulk endpoints. */
static const uint8_t configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(
        1, 1, 0, THERMO_CONFIG_LENGTH, 0, 100),

    TUD_VENDOR_DESCRIPTOR(
        0, 4, THERMO_EP_OUT, THERMO_EP_IN, CFG_TUD_VENDOR_EPSIZE)
};

/* TinyUSB requests this persistent buffer during device enumeration. */
const uint8_t* tud_descriptor_device_cb(void)
{
    return (const uint8_t*)&device_descriptor;
}

/* The descriptor index is zero-based; configuration value above is 1. */
const uint8_t* tud_descriptor_configuration_cb(uint8_t index)
{
    if (index != 0) {
        return NULL;
    }

    return configuration_descriptor;
}

/* Return USB strings as UTF-16 code units in persistent storage. */
const uint16_t* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    static uint16_t descriptor[32];
    static char serial[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
    const char* text;

    /* String zero lists supported languages, not human-readable text. */
    if (index == 0) {
        descriptor[0] = (TUSB_DESC_STRING << 8) | 4;
        descriptor[1] = 0x0409;  /* English (United States). */
        return descriptor;
    }

    /* Only the language advertised above is supported. */
    if (langid != 0x0409) {
        return NULL;
    }

    /* These indexes match the device and interface descriptors. */
    switch (index) {
    case 1:
        text = "Payman";
        break;

    case 2:
        text = "ThermoHost thermometer";
        break;

    case 3:
        pico_get_unique_board_id_string(serial, sizeof(serial));
        text = serial;
        break;

    case 4:
        text = "ThermoHost data";
        break;

    default:
        return NULL;
    }

    /* Reserve the first word for the descriptor header. */
    size_t count = strlen(text);
    if (count > 31) {
        count = 31;
    }

    /* Our strings contain ASCII only, which maps directly to UTF-16. */
    for (size_t i = 0; i < count; ++i) {
        descriptor[1 + i] = (uint8_t)text[i];
    }

    /* The low byte is the total byte length; the high byte is the type. */
    descriptor[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * count + 2));
    return descriptor;
}
