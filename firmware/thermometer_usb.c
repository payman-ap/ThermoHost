#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "tusb.h"

#define MAX6675_SO_PIN  0
#define MAX6675_CS_PIN  1
#define MAX6675_SCK_PIN 2

#define TEMPERATURE_PACKET_SIZE 10
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

/* Encode the actual sensor result using the DLL's existing wire format. */
static void encode_temperature_packet(
    uint8_t packet[TEMPERATURE_PACKET_SIZE],
    uint16_t raw,
    uint32_t timestamp_ms)
{
    bool valid = (raw & 0x0004u) == 0u;
    uint32_t temperature_x100 = 0;

    /* Avoid floating-point conversion: one sensor count is 25 hundredths C. */
    if (valid) {
        uint32_t counts = (raw >> 3) & 0x0FFFu;
        temperature_x100 = counts * 25u;
    }

    /* Fault frames carry a zero temperature that the receiver must ignore. */
    packet[0] = 0x01;
    packet[1] = valid ? 0x00 : 0x01;

    for (unsigned i = 0; i < 4; ++i) {
        packet[2 + i] = (uint8_t)(temperature_x100 >> (8u * i));
        packet[6 + i] = (uint8_t)(timestamp_ms >> (8u * i));
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

    /* Wait for an initial complete conversion without blocking USB servicing. */
    absolute_time_t next_read = make_timeout_time_ms(CONVERSION_INTERVAL_MS);

    while (true) {
        tud_task_ext(0, false);

        if (time_reached(next_read)) {
            uint16_t raw = max6675_read_raw();

            /* CS is now high: allow a full conversion before the next read. */
            next_read = make_timeout_time_ms(CONVERSION_INTERVAL_MS);

            uint8_t packet[TEMPERATURE_PACKET_SIZE];
            uint32_t timestamp_ms = to_ms_since_boot(get_absolute_time());
            encode_temperature_packet(packet, raw, timestamp_ms);

            /* Skip delivery when the host is absent or the transmit buffer is full. */
            if (tud_vendor_mounted() &&
                tud_vendor_write_available() >= sizeof(packet)) {
                uint32_t written = tud_vendor_write(packet, sizeof(packet));

                if (written == sizeof(packet)) {
                    tud_vendor_write_flush();
                }
            }
        }

        sleep_ms(1);
    }
}
