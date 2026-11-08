# 12. Control requests and the Microsoft descriptors

## Endpoint zero has a different purpose

Before a device's bulk interface can be used, the host needs descriptive and
configuration information. Control transfers on endpoint zero provide that
path. A control exchange has a setup stage, an optional data stage, and a
status stage. Your application does not manually construct the bus CRC or
handshake packets when calling `WinUsb_ControlTransfer`; the APIs and stacks
handle them. Microsoft's
[control-transfer overview](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-control-transfer)
provides the underlying model.

The setup packet's fields are `bmRequestType`, `bRequest`, `wValue`, `wIndex`,
and `wLength`. The first two are bytes; the final three are sixteen-bit fields.
The request code only has meaning together with its type and other fields.

## Decode `0xC0`

`0xC0` is binary `11000000`. Bit 7 says device-to-host. Bits 6–5 are `10`,
selecting a vendor request. Bits 4–0 are zero, selecting the device recipient.
It does not mean bulk IN endpoint `0xC0`; this field describes a control
request, not an endpoint address.

The project uses two vendor request codes in
[winusb_descriptors.c](../../firmware/usb/winusb_descriptors.c):

| Request | Additional fields | Meaning |
| --- | --- | --- |
| `0x20` | `wIndex = 7`, `wValue = 0` | Retrieve the advertised Microsoft OS 2.0 descriptor set. |
| `0x22` | `wIndex = 0`, `wValue = 0` | Retrieve the project's fresh-protocol capability reply. |

These codes are unrelated to temperature payload size even when hexadecimal
digits happen to resemble a size. The OS request code is advertised through
the BOS descriptor; Windows learns which code to use.

## BOS leads Windows to OS descriptors

The Binary Object Store descriptor advertises a Microsoft OS 2.0 platform
capability. The capability identifies the descriptor set length and vendor
request code. The set then carries a compatible ID and a registry property.
The project uses this mechanism to make a vendor-class interface discoverable
through WinUSB without its own function-driver implementation.

This is an extension mechanism, not a replacement for ordinary device,
configuration, interface, and endpoint descriptors. Those still describe the
USB interface and transport. Microsoft's
[OS-descriptor documentation](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors)
links the formal specifications if you want to audit the descriptor format
beyond this project's bytes.

## Audit the descriptor set as arithmetic

The source defines `THERMO_MS_OS_LENGTH = 0x00A2`, decimal 162. The array
contains a ten-byte set header, a twenty-byte compatible-ID feature, and a
132-byte registry-property feature. Their sum is `10 + 20 + 132 = 162`.

The compatible ID is `WINUSB`, zero padded to its fixed field width; the
sub-compatible ID is empty. The property is `DeviceInterfaceGUIDs` with
`REG_MULTI_SZ` representation. It contains the interface GUID string and the
required terminators. The descriptor applies at device scope for this
single-interface arrangement. A future composite layout would need its own
careful descriptor design rather than blind copying.

The property name occupies 42 bytes: twenty characters plus one UTF-16 null
terminator, each two bytes. The GUID text uses 38 characters with braces;
two UTF-16 zero terminators bring its data length to 80 bytes. The remaining
registry-property header fields make the feature total 132.

Why count manually when macros exist? Because a wrong length can make every
later field appear shifted. Descriptor debugging often becomes byte accounting.
`TU_VERIFY_STATIC` checks the final array lengths at compile time, catching
one important class of mistakes before flashing.

## Callback lifetime and stage handling

```c
if (stage != CONTROL_STAGE_SETUP) {
    return true;
}
```

The application examines request details during setup. TinyUSB handles the
remaining transfer progression. Returning true in later stages does not mean
the code reprocesses every stage as a new request.

The descriptor and capability arrays are static. Their storage remains valid
while TinyUSB completes the transfer. The callback returns false for setup
requests it does not recognize. It does not return an invented descriptor or
a successful temperature when the request tuple is wrong.

## Fresh capability is conditional

```c
#ifdef THERMO_FRESH_READS
```

The block protected by this compile definition replies to `0x22` with
`{'T', 'H', 2, 20, 32}`. CMake enables the definition only for the live target.
Thus the probe can bind to WinUSB using the same Microsoft descriptors while
failing the application compatibility check. Binding and capability are
independent checks by design.

The DLL zero-initializes its setup structure, assigns request type `0xC0`, code
`0x22`, and length 5. Zero initialization supplies `wValue` and `wIndex` as zero.
It also checks that exactly five bytes arrived and all five match. A successful
API call alone does not establish that the response content is acceptable.

## Debugging two kinds of control traffic

If Windows cannot bind, investigate descriptors and driver state. If the DLL
opens the device path but returns UnsupportedDevice, investigate the capability
tuple and flashed target. The same physical endpoint zero supports both
conversations, but they occur for different consumers and reasons.

The current DLL maps any capability-query failure to UnsupportedDevice, even
if the underlying cause was a control-transfer transport failure. This makes
the user-facing status simple, but loses diagnostic detail. A future logging
layer could preserve the underlying Windows error without changing the public
temperature API.

## Check your understanding

1. Decode direction, type, and recipient from `0xC0`.
2. Why is `0x20` not the fresh-read capability request?
3. Show why the OS descriptor set length is 162 bytes.
4. Why can a successful WinUSB binding coexist with UnsupportedDevice (7)?

Answers: [Chapter 12](24-answers.md#chapter-12).

<!-- NAVIGATION -->

---

[Previous: 11. The wire protocol, byte by byte](11-wire-protocol-workshop.md) · [Contents](README.md) · [Next: 13. DLL design, C linkage, and ownership](13-dll-and-ownership.md)
