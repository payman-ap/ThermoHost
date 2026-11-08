# 8. Host libraries, APIs, and resource ownership

A product library translates application operations into transport requests.
Its public interface should express device behavior without exposing every
operating-system detail. A GUI, command-line program, and test client can then
share protocol logic while choosing different presentation and scheduling
policies.

The design starts with contracts: what inputs are accepted, what happens on
failure, who owns resources, and when a result becomes valid. DLL loading is
one deployment mechanism for that library, not its entire purpose.

## API and ABI

An API specifies functions and behavior. An ABI specifies binary details such
as calling convention, symbol naming, argument representation, and layout.
C linkage can simplify exported names; it does not automatically make a
binary portable across architectures or operating systems.

Opaque handles hide implementation state. A client can retain a device handle
without knowing whether it contains Windows handles, parser buffers, or a
libusb object. However, opaqueness cannot make an arbitrary or already-freed
pointer safe. Lifetime rules remain part of the public contract.

Returning status separately from output data is useful for measurements. A
negative temperature is a legitimate number, so it should not double as a
universal error indicator. On failure, a clearly documented output-preservation
rule prevents accidental use of a partial new value.

## Opening is a sequence of acquisitions

Host access normally proceeds through discovery, selection, open/claim,
endpoint inspection, and protocol negotiation. Discovery identifies candidates;
selection chooses the intended unit. A library that always opens the first
candidate has a policy, not a general solution to multiple-device selection.

| Stage | Windows example | libusb example |
| --- | --- | --- |
| Discover | SetupAPI device-interface enumeration. | Enumerate USB devices and descriptors. |
| Select | Interface family plus product-specific policy. | VID/PID, serial, interface, and product policy. |
| Acquire | `CreateFileW`, then `WinUsb_Initialize`. | Open handle, then claim relevant interface. |
| Inspect | Query interface and pipes. | Inspect configuration/interface/endpoint descriptors. |
| Negotiate | Control transfer for capabilities. | Equivalent control-transfer tuple. |
| Release | Free WinUSB object, close Windows handle. | Release interface, close handle, release context. |

Microsoft's [WinUSB walkthrough](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/using-winusb-api-to-communicate-with-a-usb-device)
and libusb's [device-handling reference](https://libusb.sourceforge.io/api-1.0/group__libusb__dev.html)
document the respective APIs. Their handle types and buffering policies are not
interchangeable even when both implement the same instrument protocol.

## Exception-safe thinking without exceptions

Resource acquisition can fail midway. Suppose discovery succeeds, allocation
succeeds, opening succeeds, but endpoint validation fails. Every earlier
resource must still be released. Initialize resource fields to empty states,
acquire deliberately, and publish the usable object only after initialization
completes.

Automatic ownership wrappers such as `unique_ptr` can manage allocated memory;
other resources need their corresponding release operations. A Windows device
handle, WinUSB interface handle, and loaded module are three different
resources. A single generic “free” operation is not correct for all three.

| Acquisition | Release |
| --- | --- |
| `SetupDiGetClassDevsW` | `SetupDiDestroyDeviceInfoList`. |
| `CreateFileW` | `CloseHandle`. |
| `WinUsb_Initialize` | `WinUsb_Free`. |
| `LoadLibraryW` | `FreeLibrary`. |
| Private library allocation | Library's documented close/destruction operation. |

The [WinUSB initialization reference](https://learn.microsoft.com/en-us/windows/win32/api/winusb/nf-winusb-winusb_initialize)
also specifies the overlapped-capable file-handle requirement. That requirement
does not mean every subsequent operation must be used asynchronously.

## DLL loading and function pointers

Implicit linking lets the loader resolve imported functions when starting an
application. Explicit loading obtains a module and then resolves addresses
at runtime. The latter permits deliberate handling of missing modules, missing
exports, and unsupported API versions. Both require compatible binary contracts.
See Microsoft's [linking overview](https://learn.microsoft.com/en-us/cpp/build/linking-an-executable-to-a-dll?view=msvc-170).

A function pointer is only usable while its code remains loaded. Any object
requiring DLL code for destruction must be closed before unloading that code.
Similarly, asynchronous work must be completed or canceled and drained before
its callback code or buffers disappear.

## Example: the thermometer API

[ThermoDevice.h](../../ThermoHost/ThermoDevice/ThermoDevice.h) exposes an opaque
`ThermoHandle` and a status-returning read:

```cpp
THERMODEVICE_API int __cdecl Thermo_ReadTemperature(
    ThermoHandle* pHandle,
    double* pTemperatureC);
```

`Thermo_Open` clears a valid output slot, allocates empty private state,
initializes it, and publishes it only on success. Its error path uses the same
close function to release partially acquired resources. Close permits null,
but each live nonnull object must be closed exactly once.

The internal opener uses SetupAPI to find the first present matching GUID.
It retrieves a variable-length path through the common two-call pattern:
query required size, allocate, then fill. The first insufficient-buffer result
is expected. The [interface-detail reference](https://learn.microsoft.com/en-us/windows/win32/api/setupapi/nf-setupapi-setupdigetdeviceinterfacedetailw)
explains that operation.

The path is opened exclusively; WinUSB is initialized; interface setting zero
must be vendor class; both bulk directions must exist; the capability reply
must match. This is an increasing sequence of compatibility checks. The code
does not continue to a second device if the first candidate fails.

## Port the semantics, not the call spelling

A Linux implementation can retain message fields, decoder behavior, and
output-preservation rules. It must replace Windows discovery, export attributes,
time/random facilities, I/O, and cleanup with suitable counterparts. The
present parser header also includes Windows-oriented API declarations, so a
clean port would separate portable definitions first.

One-byte reads are a good example of a nonportable assumption. ThermoHost
configures WinUSB partial-read retention and disables automatic flushing. A
libusb implementation should use appropriately sized receive chunks and feed
the parser byte by byte, not assume identical OS retention behavior for
one-byte buffers. The [libusb I/O reference](https://libusb.sourceforge.io/api-1.0/group__libusb__syncio.html)
requires attention to actual transferred counts and errors.

## Keep client policy outside transport

The current console resolves exports with `LoadLibraryW` and `GetProcAddress`,
checks API version 3, opens once, reads repeatedly, then closes before unloading.
Its choice to continue after a sensor fault and stop after transport failure
is client policy. Another application could choose a different presentation
or reconnection policy while preserving the library's lifetime contract.

Exclusive device opening prevents conflicting opens under that policy; it
does not serialize threads that already share one object. Mutable parser and
pipe state require the documented per-handle serialization. Null checking is
not a substitute for synchronization or valid pointer lifetimes.

**Exercises.** Explain the distinction between API and ABI. Trace resource
cleanup when capability negotiation fails after a successful open. See
[Appendix A](appendix-a-exercises.md#chapter-8).

<!-- NAVIGATION -->

---

[Previous: 7. Application protocols and message framing](07-protocols-and-framing.md) · [Contents](README.md) · [Next: 9. State machines, partial I/O, and cancellation](09-state-machines-and-io.md)
