# 7. What WinUSB is and how it works

## The two Microsoft components

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

## Check your understanding

1. Explain the jobs of ThermoDevice.dll, Winusb.dll, and Winusb.sys.
2. Why is `ThermoHandle*` not suitable as an argument to `CloseHandle`?
3. Why does automatic driver binding not prove application-protocol compatibility?
4. What evidence shows that the current transport calls are synchronous?

Answers: [Chapter 7](24-answers.md#chapter-7).

<!-- NAVIGATION -->

---

[Previous: 6. Enumeration, descriptors, and device identity](06-enumeration-and-identity.md) · [Contents](README.md) · [Next: 8. Building, flashing, linking, and loading](08-build-flash-and-load.md)
