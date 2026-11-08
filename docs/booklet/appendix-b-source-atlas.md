# Appendix B. Software provenance and source atlas

Use this appendix while reading worked examples. The main chapters establish
concepts; this map identifies the code and tools that implement them.

## Who provides each layer?

| Component | Origin | Responsibility |
| --- | --- | --- |
| `ThermoDevice.dll`, console, protocol | This repository | Instrument operations, interpretation, client policy. |
| `Winusb.dll`, `winusb.sys` | Microsoft | Windows user API and kernel USB function-driver support. |
| SetupAPI, BCrypt | Microsoft Windows APIs | Device discovery and random-token generation. |
| `pico_stdlib`, `hardware_spi`, GPIO/time | Raspberry Pi Pico SDK | Embedded hardware/runtime interfaces. |
| TinyUSB | Ha Thach and open-source contributors | Embedded host/device USB stack; device role used here. |
| libusb | Independent open-source project | Portable host user-space USB API; not currently used by this code. |
| MSVC v142 / MSBuild | Microsoft | Compile/link/build the Windows targets. |
| GCC in the Arm-target toolchain | GNU compiler project, packaged for the target | Compile board machine code. |
| CMake | Kitware-originated open-source project | Configure/generate build rules. |
| Ninja | Open-source Ninja project | Execute generated build rules. |

The [Pico SDK repository](https://github.com/raspberrypi/pico-sdk),
[TinyUSB license](https://github.com/hathach/tinyusb/blob/master/LICENSE),
[libusb authors](https://github.com/libusb/libusb/blob/master/AUTHORS),
[CMake background](https://cmake.org/about/), [Ninja](https://ninja-build.org/),
and [GCC](https://gcc.gnu.org/) identify their respective projects. Packaging
several tools together does not give them one common author.

A header exposes declarations, an import library supports linking, and a DLL
implements runtime entry points. An embedded static library normally becomes
part of the firmware image. Thus `Winusb.lib` and `tinyusb_device` play different
deployment roles despite both appearing in link settings.

## Source landmarks

| File | Inspect | Concept |
| --- | --- | --- |
| [CMakeLists.txt](../../firmware/CMakeLists.txt) | Targets, libraries, USB stdio settings, `THERMO_FRESH_READS` | Build composition and target-specific behavior. |
| [firmware.c](../../firmware/firmware.c) | `main`, `encode_temperature_packet` | Sensor diagnostics, periodic reads, text/binary distinction. |
| [usb_probe.c](../../firmware/usb_probe.c) | `SEND_TEST_PACKETS`, fixed packet, `last_send_ms` | Known-data transport test and elapsed-time arithmetic. |
| [thermometer_usb.c](../../firmware/thermometer_usb.c) | `max6675_init`, `max6675_read_raw` | Pin routing and SPI readout. |
| [thermometer_usb.c](../../firmware/thermometer_usb.c) | Encoder, `request_bytes`, `conversion_done`, `sent` | Wire fields, command assembly, deadlines, partial writes. |
| [tusb_config.h](../../firmware/usb/tusb_config.h) | Class counts, speed, buffers | Compiled USB capabilities. |
| [usb_descriptors.c](../../firmware/usb/usb_descriptors.c) | Arrays and `tud_descriptor_*_cb` | Identity, hierarchy, persistent callback data. |
| [winusb_descriptors.c](../../firmware/usb/winusb_descriptors.c) | BOS, OS array, vendor control callback | Binding metadata and capability query. |
| [thermo_wire.h](../../protocol/thermo_wire.h) | Magic, sizes, request code | Shared constants, compared with explicit offsets. |
| [ThermoDevice.h](../../ThermoHost/ThermoDevice/ThermoDevice.h) | Exports, statuses, opaque type, contracts | API/ABI and ownership. |
| [ThermoDevice.cpp](../../ThermoHost/ThermoDevice/ThermoDevice.cpp) | `st_ThermoDevice`, `OpenUsbDevice`, exported functions | Windows discovery, handles, token requests, time budgets, cleanup. |
| [FreshResponse.h](../../ThermoHost/ThermoDevice/FreshResponse.h) | `Feed` | Rolling parser and signed fixed-point decoding. |
| [ThermoConsole.cpp](../../ThermoHost/ThermoConsole/ThermoConsole.cpp) | Loader, arguments, stop handler, loop | Runtime linking and client policy. |
| [fresh_response_test.cpp](../../tests/fresh_response_test.cpp) | Assertions and token changes | Parser contracts tested without hardware. |
| [Test runner](../../tests/run_fresh_response_tests.cmd) | Toolchain setup and compile command | Reproducible host-only check. |

`.vcxproj` files define configurations and references; `.filters` files organize
the IDE view; `.sln` groups projects. Generated `.vs`, `x64`, and firmware build
directories are outputs rather than authoritative source.

The code inspected for this edition uses root-level `ThermoHost/`, C11 firmware,
C++17 host code, Pico SDK 2.3.1, DLL API 3, and wire protocol 2. Excerpts link to
files rather than frozen line numbers. The installed SDK's TinyUSB source is
the authority for version-specific calls; online latest documentation may
describe a newer signature.

## Status reference

| Value | Status | Meaning in the current implementation |
| --- | --- | --- |
| 0 | Ok | Temperature output published. |
| 1 | InvalidArgument | Checked required pointer is null. |
| 2 | OutOfMemory | Private allocation failed. |
| 3 | InvalidPacket | Matching response has invalid checked reserved/flag fields. |
| 4 | SensorFault | Matching response reports open thermocouple. |
| 5 | DeviceNotFound | No first present matching interface found. |
| 6 | UsbError | Transport/setup or another failure mapped to this status. |
| 7 | UnsupportedDevice | Interface, endpoints, or capability not accepted. |
| 8 | Timeout | Current read did not complete under its waiting policy. |
| 9 | DeviceBusy | Exclusive open encountered a sharing violation. |

These are library statuses, not raw Windows errors. Multiple lower-level
causes can map to one value. In particular, capability transport failures
currently map to UnsupportedDevice, losing some diagnostic specificity.

## Compact terminology

| Term | Distinction to retain |
| --- | --- |
| Driver / library | OS or hardware integration / caller-facing software interface. |
| Endpoint / pipe | Device-side source or sink / host-side access relationship. |
| Packet / frame | Bus-level unit / application message, as defined in this book. |
| Token / sequence / timestamp | Command correlation / ordering / clock-relative time. |
| Freshness / latency | Acquisition relationship or age / elapsed delivery or processing time. |
| Resolution / accuracy | Representable step / closeness to physical truth. |
| Scan / sample | Group of channel values / one channel value. |
| Backpressure / overflow | Temporary inability to accept / capacity exceeded. |
| Cancellation / completion | Requested termination / established final operation state. |
| Throughput / deadline | Work per time / required completion time. |

## Boundaries intentionally left for later work

Calibration, automatic reconnect, multiple-device selection, high-rate acquisition,
and custom driver implementation are not supplied by the current application.
A Linux port and streaming protocols in the book are design examples, not
hidden repository features. Historical checkpoint notes remain useful, but
older ten-byte streaming frames and earlier API versions must not be applied
to the current exchange.

<!-- NAVIGATION -->

---

[Previous: Appendix A. Exercises and worked answers](appendix-a-exercises.md) · [Contents](README.md) · [Next: Appendix C. Further reading](appendix-c-references.md)
