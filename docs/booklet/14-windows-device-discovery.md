# 14. Discovering and opening the Windows device

## Discovery uses a Windows interface GUID

The internal `OpenUsbDevice` function in
[ThermoDevice.cpp](../../ThermoHost/ThermoDevice/ThermoDevice.cpp) begins with
the interface GUID also encoded by the firmware's Microsoft descriptors.
`SetupDiGetClassDevsW` obtains present device interfaces in that family using
`DIGCF_PRESENT | DIGCF_DEVICEINTERFACE`.

This GUID is not the general USB setup-class GUID and not the board's unique
serial number. It identifies the interface contract your application expects
to discover. Confusing those GUID roles can produce an empty search even when
Device Manager clearly shows a USB device.

## The code chooses the first result

`SetupDiEnumDeviceInterfaces` is called with index zero. If no interface exists,
`ERROR_NO_MORE_ITEMS` becomes DeviceNotFound. Other enumeration failures become
UsbError. The current implementation does not iterate through every matching
device until it finds a compatible one.

This has a practical consequence: with several devices present, the first may
be busy or incompatible while another would work. The current API exposes no
device-selection parameter. A future library could enumerate serials and let
the user choose; that is additional product behavior rather than a small
endpoint-read change.

## The two-call variable-buffer pattern

The device path is variable length. The code first calls
`SetupDiGetDeviceInterfaceDetailW` with no destination buffer to learn
`RequiredSize`. An insufficient-buffer result is expected at that stage.
It validates that result, allocates a byte array, sets the detail structure's
`cbSize`, and calls again to obtain the path.

Microsoft documents this pattern in the
[interface-detail API](https://learn.microsoft.com/en-us/windows/win32/api/setupapi/nf-setupapi-setupdigetdeviceinterfacedetailw).
The Windows API fills a buffer containing a structure header and path. The
buffer capacity is not just `sizeof` the header because the trailing path has
variable length.

`std::unique_ptr<BYTE[]>` owns this temporary storage, so all return paths
release it automatically. The device-information set is a different resource
and is destroyed explicitly. Memory ownership and operating-system handle
ownership are separate even when both are created in one function.

## Opening the interface path

```cpp
pDevice->Device = CreateFileW(
    pDetail->DevicePath,
    GENERIC_READ | GENERIC_WRITE,
    0,
    0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);
```

The desired access includes reading and writing because the protocol has both
commands and responses. Share mode zero requests exclusive access. The
creation disposition is `OPEN_EXISTING` because this is an existing device
interface, not a file to create. The wide-character function matches the
wide-character path returned by SetupAPI.

`FILE_FLAG_OVERLAPPED` supplies the handle capability WinUSB initialization
requires. The actual operations can still be synchronous, as they are here.
The official [WinUsb_Initialize reference](https://learn.microsoft.com/en-us/windows/win32/api/winusb/nf-winusb-winusb_initialize)
describes the required flag and returned interface handle.

The path buffer can be released after `CreateFileW`: the Windows handle retains
the open device relationship. It does not retain a dependency on your temporary
C++ character buffer.

## WinUSB initialization and endpoint discovery

After initialization, `WinUsb_QueryInterfaceSettings` obtains interface setting
zero. The DLL requires class `0xFF`. It then queries each endpoint, ignores
nonbulk types, and remembers the first bulk IN and first bulk OUT addresses.
Missing either produces UnsupportedDevice.

This is better than blindly assuming the endpoints are always `0x81` and
`0x01`, even though those are the current descriptors. The application uses
the discovered addresses for later I/O. It still imposes a specific policy:
one suitable interface setting and the first endpoints of each direction,
not an arbitrary composite device with many alternates.

```cpp
if ((Pipe.PipeId & 0x80) != 0)
```

The direction mask asks whether bit 7 is set. It does not inspect the low
endpoint-number bits. A bulk IN address other than `0x81` would still be
recognized by this test if the rest of the interface met the requirements.

## Capability validation follows transport validation

Finding a vendor interface and bulk endpoints proves a possible byte path.
The DLL then asks for the fresh-protocol capability reply. Only after the
exact five bytes match does the opener configure partial-read preservation
and return success.

The checks form increasing levels of specificity: matching Windows interface,
openable device, usable WinUSB object, vendor class, bulk endpoints, expected
application protocol. Diagnose the first failed stage rather than treating the
entire function as one indivisible operation.

## Failure cleanup and error detail

The helper's comment says partially acquired state must be closed on failure.
`Thermo_Open` obeys that contract. Default resource fields make cleanup safe:
the Windows handle starts as `INVALID_HANDLE_VALUE`, and the WinUSB handle
starts empty. A resource is released only when its corresponding field
indicates acquisition.

The public statuses intentionally collapse many Windows errors. A future
diagnostic build could log the failing API and its immediate `GetLastError`
value. Capture the value before unrelated API calls overwrite it. The current
enumeration code already does this before destroying its information set in
the no-interface case.

## Check your understanding

1. What happens if matching device index zero is unsupported but index one
   would work?
2. Why is insufficient buffer expected during the first detail query?
3. Why is share mode zero different from a per-handle mutex?
4. List the checks between finding a device interface and publishing a handle.

Answers: [Chapter 14](24-answers.md#chapter-14).

<!-- NAVIGATION -->

---

[Previous: 13. DLL design, C linkage, and ownership](13-dll-and-ownership.md) · [Contents](README.md) · [Next: 15. The host read: randomness, writes, and time budgets](15-host-read-and-timeouts.md)
