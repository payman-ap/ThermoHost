#include "pico/stdlib.h"
#include "tusb.h"

/* Set to 0 to test host timeouts while USB stays connected. */
#define SEND_TEST_PACKETS 1

/* Supply TinyUSB's time source using the Pico SDK clock. */
uint32_t tusb_time_millis_api(void)
{
    return to_ms_since_boot(get_absolute_time());
}

int main(void)
{
    /* Keep the connected MAX6675 deselected during this USB-only test. */
    gpio_init(1);
    gpio_put(1, 1);
    gpio_set_dir(1, GPIO_OUT);

    /* Initialize the RP2040's native USB controller in device mode. */
    const tusb_rhport_init_t usb_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_FULL
    };

    if (!tusb_init(0, &usb_init)) {
        panic("TinyUSB initialization failed");
    }

    /* Schedule test frames while continuing to service USB frequently. */
    uint32_t last_send_ms = 0;

    while (true) {
        /* Process pending USB events without waiting for another event. */
        tud_task_ext(0, false);

        uint32_t now_ms = to_ms_since_boot(get_absolute_time());

        /* Unsigned subtraction remains valid across the timestamp wrap. */
        if ((uint32_t)(now_ms - last_send_ms) >= 250u) {
            last_send_ms = now_ms;

            /* Queue a complete frame only when the interface has room. */
            if (SEND_TEST_PACKETS &&
                tud_vendor_mounted() &&
                tud_vendor_write_available() >= 10u) {
                uint8_t packet[10] = {
                    0x01, 0x00,
                    0x81, 0x10, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00
                };

                /* Insert the current device timestamp in little-endian order. */
                for (unsigned i = 0; i < 4; ++i) {
                    packet[6 + i] = (uint8_t)(now_ms >> (8u * i));
                }

                /* Copy the bytes into TinyUSB's transmit buffer. */
                uint32_t written = tud_vendor_write(packet, sizeof(packet));

                if (written == sizeof(packet)) {
                    tud_vendor_write_flush();
                }
            }
        }

        sleep_ms(1);
    }
}
