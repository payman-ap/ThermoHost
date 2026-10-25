#include "pico/stdlib.h"
#include "tusb.h"

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

    /* Process enumeration and control requests continuously. */
    while (true) {
        tud_task();
        sleep_ms(1);
    }
}
