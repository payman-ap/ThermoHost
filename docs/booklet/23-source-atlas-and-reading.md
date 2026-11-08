# 23. Source atlas and primary reading

## Use symbols as stable landmarks

Line numbers change as a project evolves. The following atlas names files and
symbols so you can navigate the current source in GitHub or an editor. Read a
whole function after studying an excerpt; omitted error paths are often the
most educational part.

| Source | Landmarks | What to learn |
| --- | --- | --- |
| [firmware/CMakeLists.txt](../../firmware/CMakeLists.txt) | `add_executable`, `target_link_libraries`, `THERMO_FRESH_READS` | Source grouping, SDK ownership, target-specific behavior. |
| [firmware.c](../../firmware/firmware.c) | `TemperatureReading`, `encode_temperature_packet`, `main` | Periodic SPI acquisition, text diagnostics, original ten-byte encoding. |
| [usb_probe.c](../../firmware/usb_probe.c) | `SEND_TEST_PACKETS`, `last_send_ms`, fixed `packet` | Known test values, elapsed arithmetic, whole-frame FIFO checks. |
| [thermometer_usb.c](../../firmware/thermometer_usb.c) | `max6675_init`, `max6675_read_raw` | Pin routing and sensor read transaction. |
| [thermometer_usb.c](../../firmware/thermometer_usb.c) | `encode_temperature_packet` | Fixed-point fields, flags, zero initialization, little-endian writes. |
| [thermometer_usb.c](../../firmware/thermometer_usb.c) | `request_bytes`, `conversion_done`, `sent` | Receive assembly, nonblocking wait, partial-write progress. |
| [tusb_config.h](../../firmware/usb/tusb_config.h) | `CFG_TUD_VENDOR`, `CFG_TUD_CDC`, buffer sizes | Compiled stack capabilities and fixed buffering. |
| [usb_descriptors.c](../../firmware/usb/usb_descriptors.c) | Device/configuration arrays and `tud_descriptor_*_cb` | Identity, endpoint layout, persistent descriptor storage. |
| [winusb_descriptors.c](../../firmware/usb/winusb_descriptors.c) | BOS, OS array, `tud_vendor_control_xfer_cb` | Windows binding metadata and application capability. |
| [thermo_wire.h](../../protocol/thermo_wire.h) | Sizes, magic, capability request | Shared constants; compare with explicit offsets in encoder/decoder. |
| [ThermoDevice.h](../../ThermoHost/ThermoDevice/ThermoDevice.h) | Export macro, enum, opaque type, contracts | Application boundary and lifetime promises. |
| [ThermoDevice.cpp](../../ThermoHost/ThermoDevice/ThermoDevice.cpp) | `st_ThermoDevice`, `OpenUsbDevice` | Private state and Windows resource acquisition. |
| [ThermoDevice.cpp](../../ThermoHost/ThermoDevice/ThermoDevice.cpp) | Exported version/open/read/close functions | Status publication, tokens, budgets, cleanup. |
| [FreshResponse.h](../../ThermoHost/ThermoDevice/FreshResponse.h) | `st_FreshResponse::Feed` | Stateful framing and signed decoding. |
| [ThermoConsole.cpp](../../ThermoHost/ThermoConsole/ThermoConsole.cpp) | Argument loop, loader calls, control handler, read loop | Client policy and shutdown ownership. |
| [fresh_response_test.cpp](../../tests/fresh_response_test.cpp) | Token changes, byte loops, assertions | Executable examples of parser guarantees. |
| [test runner](../../tests/run_fresh_response_tests.cmd) | `vcvars64.bat`, `cl`, executable invocation | Toolchain setup and test build. |

The `.vcxproj` files define Windows build configurations, toolset, project
references, and listed source/header files. `.filters` files organize the IDE
view; they do not independently compile code. `.sln` groups the two projects.
Generated `.vs`, `x64`, and build folders are not authoritative source.

## Primary sources, grouped by question

The external sources below were checked while preparing this edition. Use the
installed SDK's headers and source for exact version-dependent signatures.
The narrative applies these references to local code; diagrams and worked
examples are original explanatory material rather than copied vendor figures.

### How does the Windows side work?

- [WinUSB architecture](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/winusb-architecture): the distinction between the user-mode API DLL and kernel driver.
- [Accessing a USB device with WinUSB](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/using-winusb-api-to-communicate-with-a-usb-device): discovery, initialization, endpoint queries, and transfers.
- [WinUsb_Initialize](https://learn.microsoft.com/en-us/windows/win32/api/winusb/nf-winusb-winusb_initialize): handle prerequisites and interface lifetime.
- [WinUsb_ReadPipe](https://learn.microsoft.com/en-us/windows/win32/api/winusb/nf-winusb-winusb_readpipe): count, buffer, endpoint address, and completion arguments.
- [WinUsb_ControlTransfer](https://learn.microsoft.com/en-us/windows/win32/api/winusb/nf-winusb-winusb_controltransfer): endpoint-zero API.
- [WinUsb_SetPipePolicy](https://learn.microsoft.com/en-us/windows/win32/api/winusb/nf-winusb-winusb_setpipepolicy): policy-setting signature.
- [Pipe-policy behavior](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/winusb-functions-for-pipe-policy-modification): partial reads, excess data, and timeouts.
- [SetupDiGetDeviceInterfaceDetailW](https://learn.microsoft.com/en-us/windows/win32/api/setupapi/nf-setupapi-setupdigetdeviceinterfacedetailw): variable-size path retrieval.

### How does the USB identity and transport work?

- [USB-IF USB 2.0 specification entry](https://www.usb.org/document-library/usb-20-specification): official starting point for the base specification.
- [Infineon USB 101](https://www.infineon.com/dgdl/Infineon-AN57294_USB_101_An_Introduction_to_Universal_Serial_Bus_2.0-ApplicationNotes-v09_00-EN.pdf?fileId=8ac78c8c7cdc391c017d072d8e8e5256): vendor introduction to electrical signaling and bus packets.
- [USB endpoints and pipes](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-endpoints-and-their-pipes): device and host terminology.
- [Device descriptors](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-device-descriptors): whole-device fields and identity.
- [Configuration descriptors](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-configuration-descriptors): interface and endpoint hierarchy.
- [Bulk transfers](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-bulk-and-interrupt-transfer): direction, packet size, reliability, and scheduling tradeoffs.
- [Control transfers](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-control-transfer): setup/data/status model.
- [WinUSB device binding](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/automatic-installation-of-winusb): automatic binding mechanism and context.
- [Microsoft OS descriptors](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors): links to the formal OS descriptor specifications.

### What should I read about the board?

- [MAX6675 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/max6675.pdf): the source for sensor word layout, conversion behavior, timing, and electrical limits.
- [RP2040 datasheet](https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf): hardware architecture, boot, pin functions, SPI, and USB peripheral.
- [Pico SDK hardware APIs](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html): GPIO and SPI functions used by the firmware.
- [TinyUSB architecture](https://docs.tinyusb.org/en/latest/reference/architecture.html): controller driver, device core, class layer, and event processing.
- [TinyUSB USB concepts](https://docs.tinyusb.org/en/stable/reference/usb_concepts.html): introductory roles and classes.
- [TinyUSB integration](https://docs.tinyusb.org/en/latest/integration.html): initialization and regular task servicing.

For SDK 2.3.1 on the development machine, the installed TinyUSB implementation
was inspected under `lib/tinyusb/src/`, particularly `device/usbd.c` and
`class/vendor/vendor_device.c`. It confirms the current task-call signature and
vendor buffering behavior used in the explanation. This is an environment
reference, not a repository dependency on a new absolute path.

### What comes after this project?

- [Implicit and explicit DLL linking](https://learn.microsoft.com/en-us/cpp/build/linking-an-executable-to-a-dll?view=msvc-170): the two loading styles practiced by the project.
- [Runtime dynamic linking](https://learn.microsoft.com/en-us/windows/win32/dlls/run-time-dynamic-linking): module mapping and runtime lifetime.
- [Windows USB driver-development overview](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-driver-development-guide): client-driver layering and official sample starting points.
- [Using WDF to develop a driver](https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/using-the-framework-to-develop-a-driver): framework objects and callback model.

## Local records and their limits

[protocol.md](../protocol.md) is the current wire specification.
[fresh_reads.md](../fresh_reads.md) records current behavior and acceptance
status. [protocol_v1.md](../protocol_v1.md), [session notes](../session1.md),
and [earlier integration notes](../live_thermometer.md) preserve historical
learning stages. Historical API numbers and ten-byte frames should not be
applied to the current thirty-two-byte protocol.

The booklet's examples describe inspected source behavior. It does not add
new board verification results. When you complete a lab, record your own
firmware/host versions, procedure, expected behavior, observed behavior, and
remaining uncertainty.

<!-- NAVIGATION -->

---

[Previous: 22. Glossary and quick reference](22-glossary-and-reference-tables.md) · [Contents](README.md) · [Next: 24. Worked answers and reasoning checks](24-answers.md)
