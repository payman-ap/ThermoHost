# WinUSB interface setup (in progress)

## Identity

| Item | Value |
| --- | --- |
| Private-test VID / PID | `1209:0001` |
| Product string | `ThermoHost thermometer` |
| Descriptor revision | `bcdDevice = 0x0101` |
| Serial | Flash unique identifier, obtained through `pico_unique_id` |
| Vendor interface number | 0 |
| Bulk OUT / IN endpoints | `0x01` / `0x81` |
| Endpoint maximum packet size | 64 bytes |
| Interface GUID | `{D026972A-922E-455F-A035-B08D3E383FE4}` |
| Microsoft vendor request | `0x20`, device-to-host, device recipient |
| Descriptor request index / value | `0x0007` / `0x0000` |

VID/PID `1209:0001` is shared and reserved for private testing. It is not
universally unique and must not be used for distributed hardware.

## Files

- `firmware/usb/tusb_config.h`: one full-speed vendor-class device.
- `firmware/usb/usb_descriptors.c`: standard device, configuration, endpoint,
  and string descriptors.
- `firmware/usb/winusb_descriptors.c`: BOS, Microsoft OS 2.0 descriptor set,
  and vendor control request handling.

## How Windows will discover the interface

1. The device advertises USB version 2.1 and provides a BOS descriptor.
2. Its Microsoft OS 2.0 capability advertises request code `0x20` and a
   162-byte descriptor set.
3. Windows retrieves that set with the vendor request at index 7.
4. The compatible ID is `WINUSB`, requesting the built-in driver.
5. The `DeviceInterfaceGUIDs` REG_MULTI_SZ property registers our interface
   GUID, which the Windows DLL will later enumerate.

The single-interface probe uses device-level compatible ID and registry
properties directly under the descriptor-set header. It has no Microsoft
configuration/function subset headers. The standard USB configuration
descriptor uses `bConfigurationValue = 1`.

## Checks completed

- Verified callback signatures, descriptor macros, and vendor buffer settings
  against the installed Pico SDK 2.3.1 / TinyUSB headers and examples.
- Both descriptor C files pass ARM GCC syntax checks with `-Wall -Wextra`.
- Compile-time assertions check the 33-byte BOS and 162-byte OS descriptor set.
- Property name is 42 bytes of UTF-16LE, including its terminator.
- GUID property data is 80 bytes of UTF-16LE, including the double terminator.
- Control handler checks direction, type, recipient, request code, index,
  and value before returning the descriptor set.

## Enumeration probe build

- Added the separate `usb_probe` target, leaving the `firmware` CDC target intact.
- `usb_probe.c` initializes USB controller 0 in full-speed device mode and
  regularly calls `tud_task()`. It supplies TinyUSB's millisecond time hook
  through the Pico SDK and keeps the connected MAX6675 deselected on GP1.
- Only `usb_probe` includes `firmware/usb` for its TinyUSB configuration.
- Both stdio USB and UART are disabled for this target.
- Regenerated CMake after the new target was added; its three source files
  now appear in `build/compile_commands.json`.
- Agent verification: the ARM build completed and linked `usb_probe.elf`,
  producing `firmware/build/usb_probe.uf2`.

## First board test and descriptor-scope correction

- The user flashed the probe. Windows displayed `ThermoHost thermometer`,
  confirming that the standard device and product-string descriptors worked.
- Windows did not bind a driver. Read-only PnP inspection confirmed Code 28
  (`CM_PROB_FAILED_INSTALL`) and no WINUSB compatible ID in the device properties.
- The original 178-byte OS set used configuration/function subset headers
  adapted from a composite-device example. Our single-interface device needs
  the device-level compatible ID and property layout shown in TinyUSB's simple
  device example. Removed the two subset headers, reducing the set by 16 bytes.
- Kept VID/PID and the generated interface GUID unchanged; increased bcdDevice
  from 0x0100 to 0x0101 to permit reevaluation of the revised OS descriptors.
- The user flashed the corrected firmware and supplied a Device Manager
  screenshot showing the device under Universal Serial Bus devices without a
  warning icon. Driver Details lists `C:\Windows\System32\drivers\winusb.sys`,
  with Microsoft as the provider and Microsoft Windows as signer.
- Automatic WinUSB binding is now board-verified. The descriptor layout and
  revision changed together, so this test does not isolate the contribution
  of Windows caching versus descriptor scope.

## Windows console probe

- SetupAPI searches present interfaces using our generated interface GUID.
- The user reported successful connected and disconnected detection.
- The two-call `SetupDiGetDeviceInterfaceDetailW` pattern retrieves a
  variable-length wide-character device path. `cbSize` uses the fixed
  structure size rather than the allocation size.
- The user confirmed a path containing VID/PID 1209:0001, the board's serial,
  and our interface GUID.
- `CreateFileW` opens it with read/write access and `FILE_FLAG_OVERLAPPED`.
- `WinUsb_Initialize` succeeds on the real board.
- Interface and pipe queries report interface 0, two bulk endpoints:
  OUT 0x01 and IN 0x81, each with a maximum packet size of 64 bytes.
- All query error paths release acquired resources. Successful cleanup calls
  `WinUsb_Free` before `CloseHandle`; the SetupAPI set remains alive until
  interface queries finish and is then destroyed.
- The existing simulated DLL tests continue to pass afterward. No temperature
  data has been transferred over the real USB interface yet.

Milestone tag: `winusb-console-probe`. Diagnostic experiment branch:
`winusb-diagnostics`. Both start at this verified console-probe checkpoint;
ongoing DLL integration continues on `main`.

## Next

Move the verified discovery/open/endpoint logic behind the DLL's opaque handle,
then add firmware transmission and actual USB reads. The current board probe
only services USB requests; it does not acquire or transmit sensor frames.

## References

- https://pid.codes/1209/0001/
- https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-os-2-0-descriptors-specification
- https://developer.chrome.com/docs/capabilities/build-for-webusb
- Installed TinyUSB example: `examples/device/webusb_serial/src/`
- Simple versus composite device example: https://github.com/hathach/tinyusb/discussions/823
