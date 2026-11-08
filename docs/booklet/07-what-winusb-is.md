# 7. What WinUSB is and how it works

## The two Microsoft components

**Learning objective.** By the end of this chapter, you should be able to
explain why a generic USB driver exists, identify which component interprets
device-specific commands, and choose an appropriate access model for several
different instruments. Start with the problem the designers were solving.

## Origin: why a generic function driver was useful

A custom USB instrument does not necessarily resemble a keyboard, disk, or
speaker. Standard class drivers cannot infer what its private commands mean.
One possible architecture gives every instrument its own Windows function
driver, even when most of the driver merely forwards bulk transfers. That
creates repeated transport code and driver maintenance for many products.

Microsoft's WinUSB separates reusable USB access from product-specific logic.
The generic kernel component handles the Windows USB access role; an ordinary
application or library defines the instrument's command protocol. The
thermometer's `THQ2` request is a good example of logic that can stay outside
the kernel. This explains the design motivation; it is not a claim that every
USB device can abandon its established class or driver architecture.

Historically, WinUSB belongs to the Windows Vista generation of Windows driver
support. Microsoft documents native support in Vista and later, and an older
coinstaller-based route for 32-bit Windows XP SP2 and later. Isochronous WinUSB
APIs arrived later, in Windows 8.1. These facts explain why old tutorials can
state different feature sets. They are history, not installation advice for an
obsolete system. See Microsoft's
[driver-model comparison](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/winusb-considerations).

The useful historical shift is architectural: device developers could keep
many private command protocols in user mode while reusing a maintained USB
function driver. That choice narrows the amount of privileged code a product
team must own. It does not eliminate firmware, descriptors, permission policy,
or the need to validate every received message.

## Mechanism: reusable transport plus product policy

WinUSB provides a way for a user-mode application to communicate with a USB
device without writing its own Windows USB function driver. Its major pieces
are **Winusb.dll**, which exposes functions to applications, and **Winusb.sys**,
the kernel-mode driver serving the device. Microsoft's
[architecture description](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/winusb-architecture)
is the reference for that distinction.

Your `ThermoDevice.dll` is a third, separate component. It knows the thermometer
protocol. WinUSB does not know what `THQ2`, a thermocouple fault, or temperature
x100 means. Windows transports bytes; your code supplies their application
meaning.

This arrangement teaches substantial USB programming: discovery, descriptors,
endpoint access, control requests, policies, buffering, and error handling.
It does not make `ThermoDevice.dll` a kernel driver or bypass Windows scheduling.

## A call crosses layers

The DLL invokes `WinUsb_WritePipe` with a handle, endpoint address, and buffer.
The WinUSB system translates that request into operating-system I/O. Lower USB
layers and the host controller perform the bus communication. The board's USB
controller and TinyUSB deliver payload bytes to its vendor receive buffer.
The process reverses for a response.

You do not call `winusb.sys` as a normal exported C function. The user/kernel
boundary has OS request machinery between it. Similarly, your application does
not control the exact bus instant merely by reaching a line of C++.

## Why the device can bind automatically

The firmware includes Microsoft OS 2.0 descriptors. They advertise the
compatible ID `WINUSB` and an interface GUID property. The purpose is to allow
supported Windows systems to choose the existing WinUSB function driver and
publish an interface the application can discover. This is a firmware-assisted
binding mechanism, not a driver compiled into the RP2040 image.

Read Microsoft's [WinUSB device installation explanation](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/automatic-installation-of-winusb)
and [OS-descriptor overview](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors)
for the platform mechanism. Chapter 12 decodes this repository's descriptor
arrays. The project records that automatic binding was observed; every new
machine should still be diagnosed from its actual device state.

## What “opening” means

A device interface path is a Windows name identifying an accessible device
interface. `CreateFileW` opens it and returns a Windows `HANDLE`. The name
resembles a file path because Windows uses common I/O abstractions; no
temperature file on disk is being opened.

`WinUsb_Initialize` then produces a `WINUSB_INTERFACE_HANDLE`. This is a
different opaque object used by WinUSB APIs. The project also has its own
`ThermoHandle*`, which points to private DLL state containing both resources.
Never free one kind of handle with the destructor for another.

| Resource | Obtained by | Released by |
| --- | --- | --- |
| Device information set | `SetupDiGetClassDevsW` | `SetupDiDestroyDeviceInfoList` |
| Device path buffer | `new[]` through `unique_ptr` | `unique_ptr` array deletion |
| Windows device handle | `CreateFileW` | `CloseHandle` |
| WinUSB interface handle | `WinUsb_Initialize` | `WinUsb_Free` |
| Private device object | `new st_ThermoDevice` | `Thermo_Close` / `delete` |
| Loaded DLL module | `LoadLibraryW` | `FreeLibrary` |

The handles have related lifetimes but do not all refer to the same object.
This is why the DLL wrapper is useful: the client need not reproduce every
cleanup rule.

## The actual pipe behavior is configured

For the IN pipe, the DLL enables `ALLOW_PARTIAL_READS` and disables `AUTO_FLUSH`.
This permits the bytewise parser to consume a response without discarding the
rest of the received data. It also sets timeouts for each transfer as part of a
larger read budget. These are software policies associated with a pipe, not
sensor settings. See Microsoft's
[pipe-policy reference](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/winusb-functions-for-pipe-policy-modification).

The project intentionally favors simple parsing at low throughput. For a
large data-acquisition stream, one-byte API reads would be an expensive choice
even if most bytes were already buffered by WinUSB. You would separate chunk
acquisition from message parsing, keeping the latter correct across arbitrary
chunk boundaries.

## Synchronous calls on an overlapped-capable handle

The device is opened with `FILE_FLAG_OVERLAPPED`, as required for WinUSB
initialization. Yet the current read and write calls pass a null `OVERLAPPED`
pointer and wait synchronously. “Handle supports overlapped I/O” and “this
application pipelines asynchronous transfers” are different statements.

The flag is a prerequisite for the handle. The per-operation argument selects
how the particular call is used. Chapter 19 explains why genuine asynchronous
I/O can help responsiveness and throughput without creating hard real-time
guarantees.

## What WinUSB does not solve for you

It does not define your message framing. It does not turn a sensor-fault frame
into a valid measurement. It does not make a stale response fresh. It does not
decide which of several identical devices the user wanted. Those policies
belong to your library and application.

The current implementation opens exclusively, uses the first matching device,
and expects calls on a handle to be serialized. Those are project contracts,
not universal properties of every WinUSB application. Understanding this
boundary prevents accidental claims that a limitation of your wrapper is a
limitation of USB itself.

## Worked design example: a laboratory power supply

Consider a hypothetical USB power supply. It accepts a command to set output
voltage and returns measured voltage/current. This is an architectural example,
not a description of a product tested in this repository.

The device could expose a vendor interface with bulk OUT for commands and
bulk IN for responses. WinUSB would carry a byte sequence such as an operation
code, channel, setpoint, and command identifier. A product library would know
that a setpoint integer represents millivolts and that a status bit means
current limiting. The user interface would display values and decide what a
failed command means for the operator.

Walk one hypothetical request through these layers. The application asks
`set channel 1 to 3300 millivolts`. The library validates the channel and
range, encodes the command, and submits a write. WinUSB transfers the bytes.
Firmware validates and applies them, then returns acknowledgement. The library
matches the acknowledgement and returns a meaningful status to the UI.

Now suppose the host times out after the device applied the voltage. The host
does not know whether the action occurred. Blindly retrying a command that
increments a setpoint would act twice; retrying an idempotent “set absolute
value” command may be acceptable. WinUSB cannot choose that policy because
it does not know the command's meaning. ThermoHost's correlation token is the
beginning of this reasoning, not a complete solution for arbitrary actuators.

## Worked design example: a firmware-update interface

A hypothetical bootloader can expose a vendor interface that accepts image
blocks and returns verification results. WinUSB supplies control/bulk access;
the bootloader and host application define offsets, block identifiers, image
integrity, interruption recovery, and activation policy. A successful USB write
does not establish that an image was verified or committed to flash.

Compare this with ThermoHost. Here, `tud_vendor_write` means response bytes were
accepted into a transmit path, not that the PC displayed a temperature. In an
updater, “bytes transferred” would likewise differ from “image successfully
installed.” End-to-end completion must be defined by the application protocol.

If a device instead implements a standard update class, that class's protocol
and host tools become relevant. Choosing vendor commands is a product design
decision; it is not a requirement imposed by the shape of the USB connector.

## Real reference example: the OSR USB FX2 learning device

Microsoft's [WinUSB API walkthrough](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/using-winusb-api-to-communicate-with-a-usb-device)
uses the OSR USB FX2 learning kit as its teaching device. It walks through
opening, descriptor queries, and endpoint transfers. This is a useful primary
example of applying the same generic transport to hardware other than an
RP2040 thermometer.

Read it by correspondence rather than copying it uncritically: its device
path plays the role of our SetupAPI result, its WinUSB interface handle plays
the role of `UsbInterface`, and its endpoint queries play the role of our
discovery loop. Its device commands are not ThermoHost commands. Similar API
calls do not imply identical payload protocols.

## When a standard class is the better boundary

An ordinary keyboard normally benefits from the HID ecosystem. A USB speaker
normally benefits from the audio-class/operating-system audio path. A disk
benefits from storage integration. These examples explain why “use WinUSB for
everything” is an incomplete lesson: operating-system subsystems offer
device semantics above raw USB transfers.

For an experimental instrument with a private protocol, direct WinUSB can be
a clear starting point. For a device that should behave as a standard system
function, investigate the appropriate class first. Microsoft lists its
[provided USB class drivers](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/supported-usb-classes).
Replacing a functioning class binding merely to obtain raw access can remove
the very system integration you need.

## Transfer your understanding to Linux

There is no single Linux DLL named WinUSB that you must locate. A common
user-space route is libusb above Linux USB device access; a custom kernel
driver is another, separate route. [Chapter 25](25-linux-and-portability.md)
maps each layer and works through a ThermoHost port without assuming Windows
device paths, GUIDs, or buffering policies are portable.

## Check your understanding

1. Explain the jobs of ThermoDevice.dll, Winusb.dll, and Winusb.sys.
2. Why is `ThermoHandle*` not suitable as an argument to `CloseHandle`?
3. Why does automatic driver binding not prove application-protocol compatibility?
4. What evidence shows that the current transport calls are synchronous?

Answers: [Chapter 7](24-answers.md#chapter-7).

**Design exercise.** Assign responsibility for calibration units, transfer
retry, operating-system binding, and safe shutdown in the hypothetical power
supply. Then explain which responsibilities would remain if its host switched
from Windows to Linux. See [the extended answers](24-answers.md#extended-winusb-design).

<!-- NAVIGATION -->

---

[Previous: 6. Enumeration, descriptors, and device identity](06-enumeration-and-identity.md) · [Contents](README.md) · [Next: 8. Building, flashing, linking, and loading](08-build-flash-and-load.md)
