# Session 1: First firmware and USB console

Date: 2026-10-25

## Working setup

- Tenstar RP2040-Zero; product information specifies 2 MB external flash.
- Official Raspberry Pi Pico extension in VS Code, Pico SDK 2.3.1.
- Firmware project: `firmware/`, with C source and CMake.
- Board configuration: `waveshare_rp2040_zero`, verified in the CMake cache
  and generated SDK configuration header.
- USB stdio enabled; UART stdio disabled.
- Windows Qt kit is 64-bit, so the future Windows DLL must also be 64-bit.

## Verified on the board

1. Hold BOOT while connecting USB to enter the `RPI-RP2` boot drive.
2. Build the project to produce `firmware/build/firmware.uf2`.
3. Copy the UF2 to `RPI-RP2`; the board restarts and the drive disappears.
4. The application enumerates as USB Serial Device on COM4.
5. VS Code Serial Monitor displays `Hello, world!` repeatedly, approximately
   once per second. The user confirmed this with a screenshot.

The serial monitor was set to 115200 baud. USB CDC carries the data over USB;
this setting does not set a physical UART clock for this program.

## What the program does

- `stdio_init_all()` initializes the enabled standard I/O backends.
- `printf()` sends text through USB stdio in this configuration.
- `sleep_ms(1000)` pauses execution between messages.
- The infinite loop keeps the embedded program running.

## Roadmap adjustment

The official PlatformIO RP2040 platform documents Arduino support, rather
than a native Pico SDK framework. We chose the official Raspberry Pi Pico
VS Code extension for the native SDK and later TinyUSB work.

References:
- https://docs.platformio.org/en/latest/platforms/raspberrypi.html
- https://www.raspberrypi.com/documentation/microcontrollers/c_sdk.html

## Still untested / upcoming

- A plain GPIO LED and physical UART output have not been tested.
- The onboard LED is a WS2812 on GP16 and needs its own signaling driver.
- MAX6675 wiring and SPI acquisition are next; wiring is not yet confirmed.
- Later, replace USB stdio with the custom TinyUSB WinUSB configuration.

The main Session 1 acceptance check (build, flash, see board output) passed.
