# Project 3: RP2040-Zero + MAX6675 Thermometer (WinUSB, DLL, Qt)

Goal: the same end result as Project 1 (MAX6675 thermocouple, custom WinUSB device, your own C++ DLL, Qt 5.15 app), but on the RP2040-Zero, which flashes by drag-and-drop and has native USB. Use it to get to the Windows/DLL part faster and as a way to compare two MCU families.

Time budget: about one day per week. You can finish it faster than Project 1, since there is almost no flashing and clock-tree setup.

## Why this setup

- No ST-Link and no clock-tree work: the Pico SDK sets up the clocks and the 48 MHz USB clock for you.
- Flashing: hold BOOT, plug in USB, copy a `.uf2` file to the drive that appears (or use `picotool`/PlatformIO upload).
- The Pico SDK includes TinyUSB, so a vendor-specific WinUSB device is much less integration work than on the BluePill.
- 264 KB RAM and 133 MHz, so you can focus on the Windows side first, and learn RAM budgeting on the STM32 later.
- The Windows side (DLL, WinUSB, Qt) is identical to the STM32 projects, so you can reuse it fully.

What you give up: it is a different chip family from your STM32 soundcard goal, so the firmware itself transfers only in concepts (SPI, USB descriptors, protocol design), not in code.

## Hardware and tools

- RP2040-Zero (you have 2). Check the flash size marking on your board, since vendors differ (commonly 2 MB or 4 MB).
- MAX6675 with K-type probe
- Work PC: PlatformIO, Visual Studio 2019, Qt 5.15, Windows 11
- Windows SDK (comes with Visual Studio 2019) for `winusb.h` / `winusb.lib`. No WDK and no test-signing needed.
- Optional: use your second RP2040-Zero as a debug probe (flash the "debugprobe" firmware, then connect SWD to the first board). Your ST-Link does not debug RP2040. Without a probe, debug with printf over UART or USB.

## Wiring (proposal, verify against the RP2040-Zero pinout)

| MAX6675 | RP2040-Zero | Note |
|---|---|---|
| VCC | 3V3 | MAX6675 works from 3.0-5.5 V; stay at 3.3 V |
| GND | GND | |
| SO | GP0 (SPI0 RX / MISO) | |
| SCK | GP2 (SPI0 SCK) | |
| CS | GP1 (GPIO output) | software chip select |

The onboard RGB LED (WS2812) is usually on GP16 and is a nice status indicator (needs a PIO driver, optional).

## Key facts about the MAX6675

- Read only: CS low, clock out 16 bits, CS high.
- Bits 14..3 are the 12-bit temperature in 0.25 degC steps. Bit 2 = 1 means open thermocouple.
- Temperature = (raw >> 3) * 0.25 degC.
- Conversion takes about 0.17-0.22 s, so poll at most about 4 Hz.
- SPI mode 0, at most 4.3 MHz. Start with 1 MHz.

## Session 1: Toolchain, flashing, first life

- Create a PlatformIO project for the RP2040-Zero. Two common routes: the Arduino-Pico core (easier) or the native Pico SDK (closer to the C SDK, better for TinyUSB vendor class). For this project prefer the Pico SDK route. Check that the board definition for the RP2040-Zero exists in your PlatformIO platform version, otherwise use a generic Pico board definition and adjust the pins and flash size.
- Blink an LED on a free GPIO (the WS2812 needs PIO, so use a plain GPIO first).
- Print over USB CDC (`stdio_usb`) and over UART. Note that the SDK's USB stdio adds extra USB interfaces (for example a reset interface), so disable it later when you write your own USB descriptors.
- Learn BOOTSEL flashing and, if you set up the second board, SWD debugging.

Done when: you can build, flash, and see output from the board.

## Session 2: MAX6675 over SPI

- Use `spi_init`, `spi_read_blocking` on SPI0, with CS controlled by GPIO.
- Decode the temperature and the open-thermocouple bit.
- Print at 2-4 Hz and compare against a known temperature (touch the probe).
- Practice: keep a small struct for the last reading, avoid dynamic allocation, and use a timer (`repeating_timer` or a loop with `sleep_until`) for the 4 Hz poll.
- Reference: RP2040 datasheet (SPI chapter) and Pico SDK documentation.

Done when: reliable temperature on the serial console.

## Session 3: USB CDC and Qt app (safety net)

- Send `T=23.75\n` lines over CDC, read them in a Qt 5.15 app with `QSerialPort`, and plot them.
- This gives you a working end-to-end system before the WinUSB step. Skip it if you want to go straight to WinUSB.

## Session 4: Vendor-specific WinUSB device

- Use TinyUSB (bundled in the Pico SDK) with the vendor class: one bulk IN endpoint, optionally one bulk OUT for commands. Turn off `stdio_usb` so it does not clash.
- Write your own device, configuration and string descriptors, with your own VID/PID for testing (use the pid.codes test range or a clearly private one, and do not sell or distribute with it).
- Add the Microsoft OS 2.0 descriptors: a BOS descriptor with the MS OS 2.0 platform capability, and the vendor control request that returns the descriptor set. It contains the compatible ID `WINUSB` and a device interface GUID. Windows then binds the built-in WinUSB driver automatically.
- Look at TinyUSB's WebUSB/vendor examples for the BOS and MS OS 2.0 pattern.
- Same packet as Project 1, so the DLL is reusable: 8 bytes, little endian, `uint8 type, uint8 flags, int16 temp_x100, uint32 timestamp_ms`.
- Verify in Device Manager that it appears as a WinUSB device. If Windows caches the wrong driver after changes, change the PID or remove the device entry and re-plug.

Done when: Windows binds WinUSB automatically.

## Session 5: The DLL (C++, Visual Studio 2019)

Identical to Project 1, so reuse its API:

```c
extern "C" {
MYDEV_API int  MyDev_Open(MyDevHandle** out);
MYDEV_API int  MyDev_ReadTemperature(MyDevHandle* h, double* degC, unsigned timeout_ms);
MYDEV_API void MyDev_Close(MyDevHandle* h);
MYDEV_API const char* MyDev_ErrorString(int code);
}
```

- Find the device with SetupAPI using your interface GUID, open with `CreateFile` (overlapped), then `WinUsb_Initialize`, `WinUsb_ReadPipe`, `WinUsb_SetPipeTimeout`.
- Match DLL bitness to your Qt kit (x64 or x86).
- Return codes only, no exceptions or STL types across the DLL boundary.
- Test with a console program, then Qt via `QLibrary`.

Done when: console test and Qt app both read temperatures through your DLL.

## Session 6: Robustness and comparison

- Hot-plug and reconnect handling, worker `QThread`, command channel (for example set the poll interval), firmware version request.
- Make the DLL work with both devices: if you finished Project 1 too, let the DLL find either the BluePill or the RP2040-Zero by VID/PID. This proves your API is transport- and chip-independent.
- Write a short comparison for yourself: setup effort, flashing, debugging, USB stack integration, RAM and speed.

## Comparison with Project 1 (BluePill)

| | BluePill (STM32F103) | RP2040-Zero |
|---|---|---|
| Flashing | ST-Link over SWD | BOOTSEL drag-and-drop UF2 |
| Debugging | ST-Link, works in PlatformIO | needs a second RP2040 as debug probe, or printf |
| Clock setup | by hand (72 MHz, 48 MHz USB) | handled by the SDK |
| USB stack | TinyUSB integration by hand, or ST middleware | TinyUSB built into the SDK |
| RAM | 20 KB (tight) | 264 KB |
| Fits the STM32 soundcard goal | directly | concepts only |
| I2S | none | via PIO (not needed here) |

## Skills practiced

- Embedded: SPI, USB device descriptors, vendor class, MS OS 2.0, working with a second toolchain (Pico SDK)
- Windows: SetupAPI, WinUSB, DLL exports, 32/64-bit
- Qt 5.15: `QLibrary`, `QThread`, plotting
- Protocol and API design that is independent of the hardware

## Common problems

- Board does not mount in BOOTSEL: use a data USB cable, hold BOOT before plugging in.
- Windows does not bind WinUSB: BOS or vendor request wrong, descriptors cached, or `stdio_usb` still enabled. Change the PID to test.
- Always 0 degC or 1023.75: check wiring, CS timing, open-thermocouple bit.
- DLL does not load: bitness mismatch or missing dependency (check with `dumpbin /dependents`).
- PlatformIO board name not found for the RP2040-Zero: use a generic Pico definition and set the pins yourself.
