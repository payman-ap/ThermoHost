# 3. Enumeration, descriptors, and identity

Before an application exchanges data, the host must learn how the device is
organized. **Enumeration** is the discovery/configuration process. A
**descriptor** is structured metadata used in that process. Descriptors describe
the transport and identity; they do not by themselves define an instrument's
temperature units or private commands.

An operating system can enumerate a device successfully while an application
later rejects its protocol. Keep these two compatibility questions distinct:
“Can the OS establish device access?” and “Does the instrument implement the
contract this application expects?”

## Descriptor hierarchy

A device descriptor describes the whole device: vendor/product identifiers,
device revision, string indices, and number of configurations. A configuration
contains interfaces; interfaces have alternate settings and endpoint
descriptors. Strings provide readable names and, optionally, a serial identity.

Microsoft's [device descriptor](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-device-descriptors)
and [configuration descriptor](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-configuration-descriptors)
guides explain the hierarchy. A device can offer several configurations or
alternate settings, but a simple single-interface instrument need not use
that complexity.

Identifiers answer different questions. VID/PID identifies an advertised
product family. A serial string can distinguish physical units. A Windows
interface GUID identifies a discoverable interface family. The current USB
bus address is assigned by the host and is not a persistent product identity.
Do not use those identifiers interchangeably.

## The default control path

Descriptor retrieval uses endpoint zero. Control transfers contain a setup
stage, an optional data stage, and a status stage. A setup packet has eight
bytes represented by five fields:

| Field | Size | Purpose |
| --- | --- | --- |
| `bmRequestType` | 1 byte | Direction, request category, recipient. |
| `bRequest` | 1 byte | Operation code within that category. |
| `wValue` | 2 bytes | Operation-specific parameter. |
| `wIndex` | 2 bytes | Operation-specific index or recipient detail. |
| `wLength` | 2 bytes | Requested data-stage length. |

For `bmRequestType = 0xC0`, bit 7 selects device-to-host, bits 6–5 select a
vendor request, and recipient bits select the device. `0xC0` here is not an
endpoint address. A request's meaning depends on the full field tuple, not
only on its one-byte code. See the [control-transfer overview](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-control-transfer).

### Read a control request as concrete bytes

Consider a device-to-host vendor capability request with code `0x22`, zero
value/index, and five requested reply bytes. Its setup bytes are:

```text
C0 22 00 00 00 00 05 00
|  |  ----- ----- -----
|  |  value index length
|  request code
request type
```

The final `05 00` is little-endian length five. It does not say that the entire
control exchange is five bytes: setup is eight bytes, the data stage requests
up to five, and the status stage completes the exchange. A five-byte reply
must still be checked for content; transport success alone does not establish
that the expected protocol is present.

Host APIs usually accept these fields as values in a setup structure or
function arguments. Applications do not ordinarily append the eight bytes to
a bulk command and expect a control transfer to result. Endpoint and transfer
selection are part of the API operation as well as the payload.

## Driver selection and application capability

A standard interface class can identify a suitable system driver. Private
interfaces may use additional OS metadata or a driver package. Microsoft's
OS descriptors extend ordinary USB metadata with information Windows can use,
including a compatible driver identity and interface properties. They do not
replace the standard descriptor hierarchy.

A separate application capability request can then negotiate supported private
operations. For example, a measurement library can require a particular
protocol version and message sizes before accepting data. That lets two
firmware revisions share an OS transport identity while remaining explicitly
different application protocols.

This pattern separates stable discovery from evolving semantics. Randomly
changing identifiers to bypass a failed compatibility check makes deployment
harder; defining a deliberate capability contract makes the difference visible.

### A descriptor is a schema, not live status

Descriptors tell the host what kinds of communication are available. A
configuration can describe an interface and endpoints before any measurement
is ready. An alternate setting can offer another endpoint arrangement for
that interface; selecting one changes the active arrangement, not the meaning
of an arbitrary temperature byte by itself.

This is why enumeration belongs before ordinary application I/O. The host
needs enough structural information to build a usable access path. After that,
live commands and responses communicate changing state. Confusing descriptive
metadata with operational results makes both discovery and diagnostics harder.

## Worked example: a single vendor interface

ThermoHost's [USB descriptors](../../firmware/usb/usb_descriptors.c) advertise
VID/PID `1209:0001`, one configuration, one vendor interface, and bulk endpoint
addresses `0x01` and `0x81`. The source labels these IDs as private-test
identifiers. The device-level class is zero because class information is
provided by the interface.

```c
TUD_CONFIG_DESCRIPTOR(
    1, 1, 0, THERMO_CONFIG_LENGTH, 0, 100),

TUD_VENDOR_DESCRIPTOR(
    0, 4, THERMO_EP_OUT, THERMO_EP_IN, CFG_TUD_VENDOR_EPSIZE)
```

TinyUSB's macros construct descriptor bytes. Configuration value 1 differs
from its zero-based descriptor index 0. Interface number 0 uses string index
4; the endpoint size comes from the stack configuration. The power argument
is interpreted by the descriptor macro in milliamps. None of these arguments
defines the temperature field layout.

Descriptor callbacks return persistent storage. A pointer to a local array
would become invalid after the callback returns. The string callback uses a
static UTF-16 buffer and supports the advertised language `0x0409`. Copying
ASCII characters into 16-bit slots is adequate for these names, but is not a
general UTF-8 conversion algorithm.

## Worked example: automatic WinUSB binding

The [Microsoft descriptor source](../../firmware/usb/winusb_descriptors.c)
uses a BOS platform capability to advertise an OS 2.0 descriptor set and vendor
request code `0x20`. The set includes compatible ID `WINUSB` and property
`DeviceInterfaceGUIDs`. This supports automatic binding and Windows interface
discovery; see Microsoft's [binding explanation](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/automatic-installation-of-winusb)
and [OS-descriptor overview](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors).

The descriptor length can be audited by addition: ten-byte set header,
twenty-byte compatible-ID feature, and 132-byte property feature total 162
bytes, hexadecimal `0xA2`. The UTF-16 property name occupies 42 bytes; its GUID
data, including terminators, occupies 80. Compile-time size assertions catch
array-length mistakes before flashing.

Both vendor firmware examples use these transport descriptors. Only the live
target answers capability request `0x22` with `54 48 02 14 20`: `TH`, wire
version 2, request length 20, response length 32. Thus a probe can bind to the
right driver and still be rejected by the current application. This is the
two-level compatibility distinction introduced at the start of the chapter.

## Observe the connected device

Source describes intended descriptors; a connected device exposes the image
actually flashed. Build configuration, stale binaries, and cached OS metadata
can complicate debugging. Record observed identity and endpoints before
assuming that a successful compile changed the device.

Bootloader USB identity can also differ from application identity. A board
appearing as a programming drive during flashing does not imply its runtime
application must expose a disk or serial port.

**Exercises.** Decode `0xC0`. Explain why driver binding does not establish
private-protocol compatibility. See [Appendix A](appendix-a-exercises.md#chapter-3).

<!-- NAVIGATION -->

---

[Previous: 2. USB architecture and transfer types](02-usb-architecture.md) · [Contents](README.md) · [Next: 4. USB drivers in Windows and Linux](04-operating-system-drivers.md)
