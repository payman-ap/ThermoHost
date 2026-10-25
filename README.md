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
- Opaque handles support opening and closing a simulated device, including
  invalid open arguments and null-safe close calls.
- Handle-based temperature reads access private device state and reject null
  handles or output pointers without modifying the caller's output on failure.
- The DLL decodes a simulated 10-byte little-endian temperature frame. Valid,
  negative, fault, unknown-type, reserved-flag, and truncated-frame cases were tested.
- Custom WinUSB firmware and real USB reads through the DLL are not implemented yet.

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

Current console output:

```text
Explicitly loaded DLL API version: 2
Read status: 0
Temperature: 25 C
Null pointer status: 1
Open status: 0
Null handle status: 1
Output after rejected read: -999
Handle read status: 0
Output after handle read: 25.5
Handle temperature: 25.5 C
Null output status: 1
Device closed
Null open status: 1
Null close completed
```

The value 2 is from a DLL replacement exercise. It does not yet represent a
formally versioned production API. This exercise does not require the board.

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
- [Project 3 roadmap](docs/roadmaps/project3_rp2040_zero_thermometer.md)
- [Learning prompt](docs/prompts/codex_prompt_project3.md)

Generated binaries, build folders, and per-user IDE files are excluded from Git.
