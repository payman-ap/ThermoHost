# ThermoHost

A hands-on learning project for writing and consuming native Windows C++ DLLs,
then connecting them to an RP2040-Zero/MAX6675 thermometer through WinUSB.

The current application uses **DLL API version 3** and **wire protocol version 2**.
Flash **`thermometer_usb.uf2`** for the current Windows application. Each read
requests a new sensor conversion; the board sends no temperature stream while idle.

The learning focus is DLL exports, C linkage, opaque handles, error handling,
and DLL loading. The current console explicitly loads the DLL using
`LoadLibraryW`/`GetProcAddress`; earlier checkpoints cover implicit linking.
The DLL is a user-mode library using Microsoft's existing `winusb.sys`, not a
custom kernel driver. Qt integration and custom driver development are deferred.

For a concept-first course in USB, Windows/Linux drivers, TinyUSB, protocols,
and acquisition, start with [USB systems: from bytes to device software](docs/booklet/README.md).
Its twelve chapters use ThermoHost as a worked example, with diagrams,
calculations, exercises, and a source atlas.

## Current progress

- A native x64 DLL and console client build with Visual Studio 2019 and C++17.
- DLL loading, missing-export handling, opaque-handle ownership, and argument
  validation have been exercised through earlier learning stages.
- The DLL discovers the board and owns its Windows/WinUSB resources. The console
  accesses the device only through the DLL.
- Current live firmware restarts a MAX6675 conversion for each request, waits
  250 ms while servicing USB, and returns a response with the request's token.
- Parser tests cover stale-token rejection, fragmented frames, resynchronization,
  signed temperature decoding, sensor faults, and invalid flags.
- Recorded API-version-3 board runs passed with 3-second and 20-second idle
  pauses, including temperature changes and normal finite-run cleanup.
- Fault/recovery and Ctrl+C checks on the fresh-read version remain unconfirmed
  in the verification record. Earlier streaming tests do not verify those cases
  for the current version. See [fresh-read verification](docs/fresh_reads.md).

## Repository layout

```text
firmware/                  Pico SDK C firmware
  firmware.c               Sensor readings and packet hex text over USB CDC
  usb_probe.c              Synthetic test packets over vendor USB
  thermometer_usb.c        Fresh sensor conversions over vendor USB
  usb/                     TinyUSB configuration and shared USB descriptors
ThermoHost/             Visual Studio solution
  ThermoDevice/            Native DLL: device ownership, requests, response parsing
  ThermoConsole/           Console client: DLL loading and measurement display
protocol/thermo_wire.h     Shared request/response constants
tests/                     Host-side fresh-response parser tests
docs/                      Session notes
  roadmaps/                Project 1, 2, and 3 plans
  prompts/                 Original learning instructions, updated for this layout
Electronics_Inventory_AI_Export.csv
```

The live data path is:

```text
ThermoConsole.exe
    -> ThermoDevice.dll (exported function calls)
    -> Windows WinUSB APIs / winusb.sys
    -> RP2040 thermometer_usb firmware (USB)
    -> MAX6675 and thermocouple (SPI)
```

## Windows DLL exercise

1. Open `ThermoHost/ThermoHost.sln` in Visual Studio 2019 with the
   **Desktop development with C++** workload and a Windows SDK installed.
2. Select **Debug | x64**. Projects use MSVC v142 and C++17.
3. Flash `thermometer_usb.uf2`, build the solution, and run **ThermoConsole**.
4. Keep `ThermoDevice.dll` beside `ThermoConsole.exe` in
   `ThermoHost/x64/Debug/`.

Illustrative output for `ThermoConsole.exe --samples 3` (values and timings vary):

```text
Explicitly loaded DLL API version: 3
Fresh conversions; press Ctrl+C to stop.
Sample 1 | OK (0) | 25 C | elapsed 252 ms
Sample 2 | OK (0) | 25.25 C | elapsed 251 ms
Sample 3 | OK (0) | 25.25 C | elapsed 252 ms
Device closed after 3 read attempts.
```

The current console requires DLL API version 3 and the fresh-read firmware.
An absent matching device interface produces DeviceNotFound (5); a board running
`usb_probe` or earlier streaming firmware produces UnsupportedDevice (7).
The API version query and older simulated-temperature helper still work without
hardware, but the current console does not use simulated readings.

The `winusb-test-frame` tag preserves the working constant-frame USB transfer
and no-data timeout exercise before MAX6675 acquisition is integrated. To repeat
the timeout test, set `SEND_TEST_PACKETS` to 0 in `firmware/usb_probe.c`, rebuild
and flash the probe, and run the matching console/DLL from that checkpoint.
The current DLL rejects the probe before reading. Restore the setting to 1
afterward, and flash `thermometer_usb.uf2` to return to the current application.

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

### What each target does

`firmware/CMakeLists.txt` builds three independent RP2040 applications. Flash
**one at a time**; each replaces the application on the board. The `.uf2`, `.elf`,
and `.bin` outputs of a target are different formats of the same application.
Use the `.uf2` for the BOOT-mode copy procedure below.

| Target / UF2 | Behavior | Windows interface | Current DLL compatibility |
| --- | --- | --- | --- |
| `firmware` / `firmware.uf2` | Reads the real MAX6675 about every 250 ms. Prints raw values, temperature or thermocouple fault, and a 10-byte packet as hexadecimal text. | USB CDC serial port for a terminal. | Does not expose the DLL's vendor interface. |
| `usb_probe` / `usb_probe.uf2` | Keeps the sensor deselected. Sends a fixed 42.25 C reading every 250 ms in an older 10-byte frame, with increasing device uptime. | Vendor USB through WinUSB; no CDC console. | Rejected with UnsupportedDevice (7); use with historical probe checkpoints. |
| `thermometer_usb` / `thermometer_usb.uf2` | Starts a new sensor conversion per request, waits 250 ms, then returns a matching 32-byte response. No idle measurement stream. | Vendor USB through WinUSB; no CDC console. | Required by the current DLL. |

The original `firmware` prints packet bytes as text; it does not send them as
binary WinUSB frames. The probe isolates USB testing from sensor acquisition.
Both sensor-reading targets use SPI0 at 1 MHz with the wiring below.

The two vendor targets share VID/PID `1209:0001`, bulk OUT endpoint `0x01`, bulk
IN endpoint `0x81`, and 64-byte endpoint packet sizes. Microsoft OS 2.0
descriptors advertise WinUSB compatibility and interface GUID
`{D026972A-922E-455F-A035-B08D3E383FE4}`, which the DLL uses for discovery.
Protocol capabilities distinguish the current live firmware from the probe.

### Build and flash

Use the official **Raspberry Pi Pico** VS Code extension with Pico SDK **2.3.1**.
Open/import the existing `firmware` CMake project and select board
`waveshare_rp2040_zero`. Let the extension set up the SDK, compiler, CMake,
Ninja, and picotool; local tool paths are not committed.

From a Pico SDK terminal in the `firmware` directory, build the required target:

```powershell
cmake --build build --target thermometer_usb
```

Use `--target firmware` or `--target usb_probe` for the learning targets instead.
Hold **BOOT** while connecting the board over USB, release it, and copy
`firmware/build/<target>.uf2` to the **RPI-RP2** drive. The board restarts into
that application. Only `firmware.uf2` exposes a CDC serial port; its COM number
depends on the computer. The vendor targets communicate through WinUSB.

With USB disconnected, wire the MAX6675 as follows:

| MAX6675 | RP2040-Zero |
| --- | --- |
| VCC | 3V3 |
| GND | GND |
| SO | GP0 |
| CS | GP1 |
| SCK | GP2 |

## Live thermometer

Build the sensor firmware from the `firmware` directory in the Pico SDK terminal:

```powershell
cmake --build build --target thermometer_usb
```

Flash `firmware/build/thermometer_usb.uf2` through BOOT mode, with the MAX6675
wired as above, and build/run the Windows console (DLL API version 3).
From `ThermoHost/x64/Debug`:

```powershell
.\ThermoConsole.exe --samples 3
.\ThermoConsole.exe --samples 3 --interval-ms 3000
.\ThermoConsole.exe
```

The default is continuous reading until Ctrl+C. `--samples N` sets a positive
read-attempt limit, including sensor-fault results. `--interval-ms N` adds a
pause of 0 through 60000 ms between attempts, excluding the final attempt of a
finite run. That pause is excluded from displayed read durations. Use `--help`
for usage.

A disconnected thermocouple returns SensorFault (4) without changing the caller's
temperature output. The console prints the fault and continues reading; other
read failures, including timeouts and USB errors, end the session with a nonzero
exit code. Ctrl+C requests a stop after any active read finishes, then the
console closes the device before unloading the DLL. Automatic reconnection is
not implemented. Restore wiring with USB disconnected.

The `live-thermometer-winusb` tag preserves the earlier tested streaming system.
Current code removes its backlog through request/response acquisition. New DLL
and firmware must be used together. See [fresh-read verification](docs/fresh_reads.md)
for commands, remaining board checks, and initial-phase completion criteria.

## How the DLL gets a fresh reading

`ThermoDevice.h` exposes C-linkage functions and an opaque `ThermoHandle`:

| Export | Purpose |
| --- | --- |
| `Thermo_GetApiVersion()` | Returns 3, independently of wire protocol version 2. |
| `Thermo_GetSimulatedTemperature()` | Returns a fixed 25.0 C without hardware; retained for compatibility. |
| `Thermo_Open()` | Discovers the first present matching interface, opens it exclusively, finds bulk endpoints, and checks protocol capabilities. |
| `Thermo_ReadTemperature()` | Requests a new conversion and returns a status plus degrees Celsius on success. |
| `Thermo_Close()` | Frees WinUSB resources, closes the Windows handle, and deletes private state. Null is allowed. |

At open, the DLL sends vendor control request `0x22`. Live firmware returns
`54 48 02 14 20`: `TH`, protocol version 2, request size 20, response size 32.
Firmware without this capability is rejected. A sharing violation when opening
the device returns DeviceBusy (9).

Each `Thermo_ReadTemperature()` call performs this exchange:

1. The DLL generates a random 16-byte token and writes a **20-byte request** to
   bulk OUT: ASCII `THQ2` followed by the token.
2. After receiving the command, the RP2040 restarts the MAX6675 conversion and
   waits **250 ms**, continuing to service USB events.
3. The firmware reads the sensor and sends a **32-byte response** on bulk IN:
   ASCII `THR2`, fault flags and reserved bytes, temperature x100, device uptime
   at the sensor read, and the echoed token. Numeric fields are little endian.
4. The DLL assembles the response across USB reads, skips noise and responses
   with old tokens, checks flags, and converts temperature x100 to a `double`
   in degrees Celsius.

This produces a conversion started after the command instead of retrieving a
queued sample. Normal reads take about 250 ms plus transport/scheduling overhead,
including after idle pauses. A late response from a timed-out read cannot
normally satisfy a later request because its token differs. The token provides
correlation, not authentication.

The DLL applies one **1000 ms budget** to request writing and response assembly.
SensorFault (4), InvalidPacket (3), Timeout (8), and USB failures leave the
caller's temperature output unchanged. Timestamps are not exposed by the public
API. Serialize operations on each handle, do not close during a read, and close
each live handle once before unloading the DLL.

See [wire protocol version 2](docs/protocol.md) for exact byte offsets.
The branch `fresh-read-snapshot` and tag `fresh-read-request-response-v1`
preserve this request/response implementation.

Host-side parser tests can be built and run from the repository root:

```powershell
.\tests\run_fresh_response_tests.cmd
```

The script currently uses the VS2019 Professional `vcvars64.bat` path; adjust
it for another installation. These tests do not exercise hardware or USB.

## Learning notes

- [Session 1: first firmware and USB console](docs/session1.md)
- [Session 2: MAX6675 acquisition](docs/session2.md)
- [DLL basics and verified failure tests](docs/dll_basics.md)
- [Binary packet specification](docs/protocol.md)
- [Earlier 10-byte streaming protocol](docs/protocol_v1.md)
- [Firmware packet encoding and board tests](docs/firmware_packets.md)
- [WinUSB enumeration probe and driver binding](docs/winusb_setup.md)
- [MAX6675 over WinUSB integration](docs/live_thermometer.md)
- [Fresh-read behavior and verification](docs/fresh_reads.md)
- [Project 3 roadmap](docs/roadmaps/project3_rp2040_zero_thermometer.md)
- [Learning prompt](docs/prompts/codex_prompt_project3.md)

Generated binaries, build folders, and per-user IDE files are excluded from Git.
