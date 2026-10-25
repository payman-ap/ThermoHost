#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

#define MAX6675_SO_PIN  0
#define MAX6675_CS_PIN  1
#define MAX6675_SCK_PIN 2

#define TEMPERATURE_PACKET_SIZE 10

typedef struct {
    uint16_t raw;
    double temperature_c;
    bool valid;
} TemperatureReading;

/* Encode one reading into a caller-provided 10-byte buffer. */
static void encode_temperature_packet(
    uint8_t packet[TEMPERATURE_PACKET_SIZE],
    const TemperatureReading* reading,
    uint32_t timestamp_ms)
{
    /* Fault frames keep this field zero; the flag tells the DLL to ignore it. */
    uint32_t temperature_x100 = 0;

    if (reading->valid) {
        /* Each sensor count is 0.25 C, or 25 hundredths of a degree. */
        uint32_t counts = (reading->raw >> 3) & 0x0FFFu;
        temperature_x100 = counts * 25u;
    }

    /* Set the message type and open-thermocouple flag. */
    packet[0] = 0x01;
    packet[1] = reading->valid ? 0x00 : 0x01;

    /* Write temperature to bytes 2..5 and timestamp to bytes 6..9, low byte first. */
    for (unsigned i = 0; i < 4; ++i) {
        packet[2 + i] = (uint8_t)(temperature_x100 >> (8u * i));
        packet[6 + i] = (uint8_t)(timestamp_ms >> (8u * i));
    }
}

int main(void)
{
    stdio_init_all();

    // Set the output value before enabling the output driver.
    gpio_init(MAX6675_CS_PIN);
    gpio_put(MAX6675_CS_PIN, 1);
    gpio_set_dir(MAX6675_CS_PIN, GPIO_OUT);

    uint actual_baud = spi_init(spi0, 1000000);
    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(MAX6675_SO_PIN, GPIO_FUNC_SPI);
    gpio_set_function(MAX6675_SCK_PIN, GPIO_FUNC_SPI);

    printf("SPI0 initialized: %u Hz\n", actual_baud);

    TemperatureReading last_reading = {0};
    absolute_time_t next_read = make_timeout_time_ms(250);

    while (true) {
        // CS is high here: allow a complete temperature conversion.
        sleep_until(next_read);

        uint8_t bytes[2];

        gpio_put(MAX6675_CS_PIN, 0);
        sleep_us(1);  // Allow the sensor output to become ready.

        spi_read_blocking(spi0, 0, bytes, sizeof(bytes));

        sleep_us(1);
        gpio_put(MAX6675_CS_PIN, 1);

        next_read = make_timeout_time_ms(250);

        // The sensor sends the high byte first.
        uint16_t raw = ((uint16_t)bytes[0] << 8) | bytes[1];

        last_reading.raw = raw;
        last_reading.valid = (raw & 0x0004u) == 0u;
        last_reading.temperature_c = 0.0;

        if (last_reading.valid) {
            uint16_t counts = (raw >> 3) & 0x0FFFu;
            last_reading.temperature_c = counts * 0.25;

            printf("Raw: 0x%04X | Temperature: %.2f C\n",
                (unsigned int)last_reading.raw,
                last_reading.temperature_c);
        } else {
            printf("Raw: 0x%04X | Thermocouple disconnected\n",
                (unsigned int)last_reading.raw);
        }

        /* Capture the device timestamp and encode the latest reading. */
        uint8_t packet[TEMPERATURE_PACKET_SIZE];
        uint32_t timestamp_ms = to_ms_since_boot(get_absolute_time());

        encode_temperature_packet(packet, &last_reading, timestamp_ms);

        /* Display the encoded bytes for inspection before adding USB transport. */
        printf("Packet:");
        for (unsigned i = 0; i < TEMPERATURE_PACKET_SIZE; ++i) {
            printf(" %02X", (unsigned int)packet[i]);
        }
        printf("\n");
    }
}
