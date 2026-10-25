# ThermoHost

A hands-on learning project for writing and consuming native Windows C++ DLLs,
then connecting them to an RP2040-Zero/MAX6675 thermometer through WinUSB.

The current priority is DLL exports, C interfaces, error handling, and both
implicit and explicit loading. Qt integration is deferred.

## Current progress

- RP2040 firmware reads the MAX6675 over SPI and prints temperature over USB CDC.
- Open-thermocouple detection and approximately 4 Hz polling have been tested.
- A native x64 DLL and console client build with Visual Studio 2019 and C++17.
- Implicit linking and explicit loading with `LoadLibraryW`/`GetProcAddress` work.
- Missing-DLL and missing-export error handling have been tested.
- Simulated-temperature reads return status codes and write through an output
  parameter; successful reads and null-pointer rejection have been tested.
- Opaque-handle lifetime, invalid open arguments, and null-safe Close were
  verified first with simulation and then with real USB resources.
- Handle-based temperature reads access private device state and reject null
  handles or output pointers without modifying the caller's output on failure.
- The 10-byte little-endian decoder was tested with valid, negative, fault,
  unknown-type, reserved-flag, and truncated-frame inputs.
- Firmware encodes the same frames and prints them as hex text over USB CDC;
  real temperature and open-thermocouple packets were verified on the board.
- The separate `usb_probe` firmware enumerates as a vendor device and Windows
  automatically binds Microsoft's `winusb.sys`; verified in Device Manager.
- The console discovers the real device, retrieves its path, initializes WinUSB,
  and verifies two 64-byte bulk endpoints (OUT 0x01 and IN 0x81).
- The DLL now discovers and opens the real board, owns its Windows/WinUSB
  handles, and remembers its bulk endpoints. Connected and absent-board cases
  were tested.
- The board transmits known 42.25 C test frames over bulk IN, and the DLL reads
  and decodes them successfully. Real sensor acquisition over WinUSB is next.
- Twenty consecutive real USB reads passed. Disabling board transmission while
  keeping USB connected produced timeout status 8 after 1000 ms with output unchanged.

## Repository layout

```text
firmware/                  Pico SDK C firmware
dll/ThermoHost/             Visual Studio solution
  ThermoDevice/            Native DLL
  ThermoConsole/           Console client
docs/                      Session notes
  roadmaps/                Project 1, 2, and 3 plans
  prompts/                 Original learning instructions, updated for this layout
Electronics_Inventory_AI_Export.csv
```

## Windows DLL exercise

1. Open `dll/ThermoHost/ThermoHost.sln` in Visual Studio 2019 with the
   **Desktop development with C++** workload and a Windows SDK installed.
2. Select **Debug | x64**. Projects use MSVC v142 and C++17.
3. Build the solution and run **ThermoConsole**.
4. Keep `ThermoDevice.dll` beside `ThermoConsole.exe` in
   `dll/ThermoHost/x64/Debug/`.

Example DLL portion of console output (read timings vary):

```text
Explicitly loaded DLL API version: 2
Read status: 0
Temperature: 25 C
Null pointer status: 1
Open status: 0
Null handle status: 1
Output after rejected read: -999
USB sample 1 | status 0 | 42.25 C | elapsed 0 ms
...
USB sample 20 | status 0 | 42.25 C | elapsed 250 ms
Null output status: 1
Device closed
Null open status: 1
Null close completed
```

The value 2 is from a DLL replacement exercise. It does not yet represent a
formally versioned production API. The handle-based exercise now requires the
board running `usb_probe.uf2`: without it, Open returns DeviceNotFound (5).
The older fixed-value helper and API version query still work without hardware.

The `winusb-test-frame` tag preserves the working constant-frame USB transfer
and no-data timeout exercise before MAX6675 acquisition is integrated. To repeat
the timeout test, set `SEND_TEST_PACKETS` to 0 in `firmware/usb_probe.c`, rebuild
and flash the probe, and run the console. Restore it to 1 and reflash afterward.

## Packet experiment checkpoint

`packet-playground` starts at the tested decoder checkpoint, before WinUSB
hardware integration. To experiment in a separate checkout:

```powershell
git clone --branch packet-playground https://github.com/payman-ap/ThermoHost.git ThermoHost-packets
```

Continue packet experiments on that branch; hardware integration continues on
`main`. The tag `packet-decoder-snapshot` preserves the original checkpoint even
after either branch acquires more commits. The snapshot contains source and notes;
build the binaries locally using the instructions above.

## WinUSB diagnostic checkpoint

The `winusb-console-probe` tag preserves the verified device discovery,
WinUSB initialization, and endpoint inspection exercise before USB ownership
moves into the DLL. Branch `winusb-diagnostics` starts at the same checkpoint
for independent hardware diagnostic experiments.

```powershell
git clone --branch winusb-diagnostics https://github.com/payman-ap/ThermoHost.git ThermoHost-usb-diagnostics
```

Build the Windows solution locally, and use `usb_probe.uf2` on the board for
this exercise. The board checks use real USB; the subsequent DLL temperature
checks still use the simulated packet.

## Firmware

Use the official **Raspberry Pi Pico** VS Code extension with Pico SDK **2.3.1**.
Open/import the existing `firmware` CMake project and select board
`waveshare_rp2040_zero`. Let the extension set up the SDK, compiler, CMake,
Ninja, and picotool; local tool paths are not committed.

Compile to produce `firmware/build/firmware.uf2`. Hold **BOOT** while connecting
the board over USB, release it, and copy the UF2 to the **RPI-RP2** drive. The
board restarts and exposes a USB CDC serial port. Its COM number depends on
the computer; COM4 was used during development.

With USB disconnected, wire the MAX6675 as follows:

| MAX6675 | RP2040-Zero |
| --- | --- |
| VCC | 3V3 |
| GND | GND |
| SO | GP0 |
| CS | GP1 |
| SCK | GP2 |

## Learning notes

- [Session 1: first firmware and USB console](docs/session1.md)
- [Session 2: MAX6675 acquisition](docs/session2.md)
- [DLL basics and verified failure tests](docs/dll_basics.md)
- [Binary packet specification](docs/protocol.md)
- [Firmware packet encoding and board tests](docs/firmware_packets.md)
- [WinUSB enumeration probe and driver binding](docs/winusb_setup.md)
- [Project 3 roadmap](docs/roadmaps/project3_rp2040_zero_thermometer.md)
- [Learning prompt](docs/prompts/codex_prompt_project3.md)

Generated binaries, build folders, and per-user IDE files are excluded from Git.
