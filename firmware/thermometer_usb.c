#include <stdint.h>
#include <string.h>
#include "../protocol/thermo_wire.h"

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "tusb.h"

#define MAX6675_SO_PIN  0
#define MAX6675_CS_PIN  1
#define MAX6675_SCK_PIN 2

#define TEMPERATURE_PACKET_SIZE THERMO_RESPONSE_SIZE
#define CONVERSION_INTERVAL_MS 250

/* Supply the same clock hook as the known-frame USB probe. */
uint32_t tusb_time_millis_api(void)
{
    return to_ms_since_boot(get_absolute_time());
}

/* Configure the sensor interface with CS high so conversion can run. */
static void max6675_init(void)
{
    gpio_init(MAX6675_CS_PIN);
    gpio_put(MAX6675_CS_PIN, 1);
    gpio_set_dir(MAX6675_CS_PIN, GPIO_OUT);

    spi_init(spi0, 1000000);
    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(MAX6675_SO_PIN, GPIO_FUNC_SPI);
    gpio_set_function(MAX6675_SCK_PIN, GPIO_FUNC_SPI);
}

/* Read all 16 bits under one CS assertion, then start the next conversion. */
static uint16_t max6675_read_raw(void)
{
    uint8_t bytes[2];

    gpio_put(MAX6675_CS_PIN, 0);
    sleep_us(1);
    spi_read_blocking(spi0, 0, bytes, sizeof(bytes));
    sleep_us(1);
    gpio_put(MAX6675_CS_PIN, 1);

    return ((uint16_t)bytes[0] << 8) | bytes[1];
}

/* Echo the request token so delayed replies cannot satisfy a later read. */
static void encode_temperature_packet(
    uint8_t packet[TEMPERATURE_PACKET_SIZE],
    uint16_t raw,
    uint32_t timestamp_ms,
    const uint8_t token[THERMO_TOKEN_SIZE])
{
    bool valid = (raw & 0x0004u) == 0u;
    uint32_t temperature_x100 = 0;

    /* Avoid floating-point conversion: one sensor count is 25 hundredths C. */
    if (valid) {
        uint32_t counts = (raw >> 3) & 0x0FFFu;
        temperature_x100 = counts * 25u;
    }

    /* Fault frames carry a zero temperature that the receiver must ignore. */
    memset(packet, 0, TEMPERATURE_PACKET_SIZE);
    memcpy(packet, THERMO_RESPONSE_MAGIC, 4);
    packet[4] = valid ? 0x00 : 0x01;
    memcpy(packet + 16, token, THERMO_TOKEN_SIZE);

    for (unsigned i = 0; i < 4; ++i) {
        packet[8 + i] = (uint8_t)(temperature_x100 >> (8u * i));
        packet[12 + i] = (uint8_t)(timestamp_ms >> (8u * i));
    }
}

int main(void)
{
    max6675_init();

    /* The same USB descriptors allow the existing DLL to find this firmware. */
    const tusb_rhport_init_t usb_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_FULL
    };

    if (!tusb_init(0, &usb_init)) {
        panic("TinyUSB initialization failed");
    }

    uint8_t request[THERMO_REQUEST_SIZE];
    unsigned request_bytes = 0;
    uint8_t packet[TEMPERATURE_PACKET_SIZE];
    uint8_t token[THERMO_TOKEN_SIZE];
    bool converting = false;
    bool response_ready = false;
    unsigned sent = 0;
    absolute_time_t conversion_done = nil_time;

    while (true) {
        tud_task_ext(0, false);

        if (!tud_vendor_mounted()) {
            request_bytes = 0;
            converting = false;
            response_ready = false;
        } else {
            /* Assemble commands, resynchronizing after an incomplete old command. */
            while (tud_vendor_available()) {
                if (tud_vendor_read(request + request_bytes, 1) != 1) {
                    break;
                }
                ++request_bytes;
                if (request_bytes >= 4 &&
                    memcmp(request, THERMO_REQUEST_MAGIC, 4) != 0) {
                    memmove(request, request + 1, --request_bytes);
                }
                if (request_bytes == sizeof(request)) {
                    memcpy(token, request + 4, sizeof(token));
                    request_bytes = 0;
                    /* Restart conversion AFTER this request, not from queued data. */
                    gpio_put(MAX6675_CS_PIN, 0);
                    sleep_us(1);
                    gpio_put(MAX6675_CS_PIN, 1);
                    conversion_done = make_timeout_time_ms(CONVERSION_INTERVAL_MS);
                    converting = true;
                    response_ready = false;
                }
            }

            /* Service USB throughout the conversion wait. */
            if (converting && time_reached(conversion_done)) {
                uint16_t raw = max6675_read_raw();
                encode_temperature_packet(packet, raw,
                    to_ms_since_boot(get_absolute_time()), token);
                converting = false;
                response_ready = true;
                sent = 0;
            }

            /* Send one reply per request; remember any partial FIFO write. */
            if (response_ready) {
                sent += tud_vendor_write(packet + sent, sizeof(packet) - sent);
                tud_vendor_write_flush();
                if (sent == sizeof(packet)) {
                    response_ready = false;
                }
            }
        }
        sleep_ms(1);
    }
}
