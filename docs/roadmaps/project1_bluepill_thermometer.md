# Project 1: BluePill + MAX6675 Thermometer (WinUSB, DLL, Qt)

Goal: read a K-type thermocouple with a MAX6675 on a WeAct BluePill (STM32F103CBT6), send the data to Windows as a custom WinUSB device, wrap it in your own C++ DLL, and show it in a Qt 5.15 app.

Time budget: about one day per week. Each "session" below is roughly one day. You can combine or split them.

## Hardware and tools

- WeAct BluePill (STM32F103CBT6, 128 KB flash, 20 KB RAM)
- MAX6675 module with K-type probe (you confirmed it is complete)
- ST-LINK V2 (SWD flashing and debugging; the BluePill has no USB bootloader)
- Optional: 24 MHz logic analyzer to look at the SPI lines
- Work PC: PlatformIO, Visual Studio 2019, Qt 5.15, Windows 11
- Windows SDK (installed with Visual Studio 2019) provides `winusb.h` and `winusb.lib`. No WDK and no test-signing mode needed.

## Wiring (proposal, verify against the board pinout)

| MAX6675 | BluePill | Note |
|---|---|---|
| VCC | 3.3 V | MAX6675 works from 3.0-5.5 V; stay at 3.3 V |
| GND | GND | |
| SCK | PA5 (SPI1_SCK) | |
| SO | PA6 (SPI1_MISO) | |
| CS | PA4 (GPIO output) | software chip select |

USB goes to PA11 (D-) and PA12 (D+). Check that your WeAct board has a correct USB pull-up on D+.

## Key facts about the MAX6675

- You only read it (no commands). Pull CS low, clock out 16 bits, release CS.
- Bit 15 is a dummy zero. Bits 14..3 are the 12-bit temperature, in 0.25 degC steps. Bit 2 is 1 if the thermocouple is open. Bit 1 is the device ID. Bit 0 is three-state.
- Temperature = (raw >> 3) * 0.25 degC.
- A conversion takes about 0.17-0.22 s, so poll at most about 4 Hz. Reading faster just returns the same value.
- SPI mode 0, maximum clock 4.3 MHz. Use a slower clock such as 1 MHz to start.

## Session 1: Toolchain and first life

- Create the PlatformIO project: board `bluepill_f103c8_128k`, upload and debug with `stlink`.
- Framework: `stm32cube` (HAL). Set up the clock tree by hand to 72 MHz from the HSE crystal. USB needs a 48 MHz clock, so the PLL must give exactly 72 MHz with the USB prescaler set to 1.5.
- Blink the LED, then output debug text over a UART with your FT232RL or CP2102 adapter.
- Learn the debugger: breakpoints, watch variables, look at the registers.
- Reference: STM32F103 reference manual RM0008 (RCC, GPIO chapters).

Done when: LED blinks, UART text appears, you can single-step in the debugger.

## Session 2: MAX6675 over SPI

- Configure SPI1 as master, mode 0, 16-bit or 2x8-bit, slow clock. Use GPIO for CS.
- Read the raw 16 bits, decode the temperature and the open-thermocouple flag.
- Print the value over UART at about 2-4 Hz.
- Check the SPI signals with the logic analyzer once to see CS, clock and data.
- Memory practice: keep the last reading in a small struct, no dynamic allocation. Look at the map file and see how much RAM and flash you use.
- Reference: RM0008 SPI chapter.

Done when: the temperature on UART matches reality (touch the probe, it should rise).

## Session 3: USB CDC and Qt app

- Add TinyUSB (as a library or git submodule) for the STM32F1 FSDEV USB driver, and start with the CDC class so the BluePill shows up as a COM port. If TinyUSB integration into PlatformIO costs too much time, use the ST USB device middleware for CDC as a fallback.
- Send one line per reading, for example `T=23.75\n`.
- Build a small Qt 5.15 app (Visual Studio 2019 / MSVC2019 kit) that reads the port with `QSerialPort` and shows the value and a live plot.
- This gives you a working end-to-end system before the harder WinUSB step.

Done when: the Qt app shows the live temperature from the BluePill.

## Session 4: Vendor-specific WinUSB device

- Replace CDC with a vendor-specific USB interface: one bulk IN endpoint (and optionally one bulk OUT for commands).
- Define a simple binary protocol. Example packet, 8 bytes: `uint8 type, uint8 flags, int16 temp_x100, uint32 timestamp_ms`. Use little endian and fixed-size packed structs.
- Add the Microsoft OS 2.0 descriptor set (BOS descriptor with the MS OS 2.0 platform capability, plus a vendor request to deliver it). It contains a compatible ID `WINUSB` and a device interface GUID. Windows then loads the built-in WinUSB driver automatically, with no INF file.
- Plug it in and check in Device Manager that it appears under "Universal Serial Bus devices" as a WinUSB device.
- Reference: TinyUSB vendor class and WebUSB/MS OS 2.0 examples, USB 2.0 spec chapter 9.
- If Windows caches the wrong driver after you change descriptors, change the product string or PID, or remove the device in Device Manager and re-plug.

Done when: Windows binds WinUSB without manual driver installation.

## Session 5: The DLL (C++, Visual Studio 2019)

- Create a DLL project with the same bitness (x64 or x86) as your Qt kit. Mismatch means the load fails.
- Use a C API so it is stable and easy to call from Qt and other languages:

```c
#ifdef MYDEV_EXPORTS
#define MYDEV_API __declspec(dllexport)
#else
#define MYDEV_API __declspec(dllimport)
#endif

typedef struct MyDevHandle MyDevHandle;

extern "C" {
MYDEV_API int  MyDev_Open(MyDevHandle** out);
MYDEV_API int  MyDev_ReadTemperature(MyDevHandle* h, double* degC, unsigned timeout_ms);
MYDEV_API void MyDev_Close(MyDevHandle* h);
MYDEV_API const char* MyDev_ErrorString(int code);
}
```

- Inside: find the device with SetupAPI (`SetupDiGetClassDevs`, `SetupDiEnumDeviceInterfaces`) using your interface GUID, open it with `CreateFile` (overlapped), then `WinUsb_Initialize`, `WinUsb_ReadPipe`, `WinUsb_SetPipeTimeout`.
- Use return codes, not exceptions, across the DLL boundary. Do not return C++ objects or STL types.
- Handle: device unplugged (read returns an error), timeouts, opening twice.
- Test with a small console program first, then with Qt via `QLibrary` (resolve the functions with `resolve`). Also try linking with the import library.

Done when: a console test and the Qt app both read temperatures through your DLL.

## Session 6: Polish and robustness

- Hot-plug: the app reconnects when the device is plugged in again.
- Add a command channel (for example set the sample interval) over a control transfer or a bulk OUT endpoint.
- Add a version/status request so the DLL can check firmware compatibility.
- Put reading in a worker `QThread` so the Qt GUI never blocks.
- Write a short README with the protocol and API.
- Optional: unit tests for the packet decoder, and logging to CSV.

Done when: you can unplug and re-plug the device while the app runs and it recovers.

## Skills practiced

- Embedded: clock tree, GPIO, SPI, USB device stack, linker map and RAM budget, debugging with SWD
- Windows: SetupAPI, WinUSB API, DLL exports, calling conventions, 32/64-bit
- Qt 5.15: `QLibrary`, `QThread`, plotting
- Protocol design: packed structs, endianness, versioning

## Common problems

- Device not recognized: wrong clock (USB needs exactly 48 MHz), bad D+ pull-up, or a power-only cable.
- Windows uses the wrong driver: descriptors were cached, change the PID or remove the device entry.
- Always 0 degC or 1023.75: check wiring, CS timing and the open-thermocouple bit.
- DLL fails to load: bitness mismatch or missing dependency (check with `dumpbin /dependents`).
