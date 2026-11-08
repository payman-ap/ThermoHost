# 13. DLL design, C linkage, and ownership

## The library boundary is part of the design

The DLL gives a Windows application a small thermometer-oriented API. The
client asks to open, read, and close; it does not allocate endpoint state or
parse packets. This is useful encapsulation even with only one console client.
A future GUI or another language binding can reuse the same conceptual
interface.

Read [ThermoDevice.h](../../ThermoHost/ThermoDevice/ThermoDevice.h). The
`THERMODEVICE_BUILD` definition selects `__declspec(dllexport)` while compiling
the DLL and `__declspec(dllimport)` for clients. These attributes affect symbol
export/import handling. They are not USB configuration directives.

The `extern "C"` block requests C language linkage for exported function
names. It avoids C++ overload-based name decoration at that boundary, but does
not make every signature automatically binary-compatible across all
architectures and calling conventions. The explicit `__cdecl`, pointer sizes,
and matching build architecture still matter.

## Status and output are separate channels

```cpp
THERMODEVICE_API int __cdecl Thermo_ReadTemperature(
    ThermoHandle* pHandle,
    double* pTemperatureC);
```

An integer status reports whether the operation succeeded. A writable output
pointer receives degrees Celsius only on success. This avoids assigning a
special temperature such as -999 to mean an error. The console uses -999 as
an initial sentinel for demonstration, not as part of the public protocol.

The exported simulation helper follows the same idea: it rejects a null output
pointer and otherwise writes 25.0. It is retained from the early DLL exercise;
the current console does not use it to obtain fallback readings after a USB
failure.

The status enumeration assigns values zero through nine. Success, invalid
argument, allocation failure, invalid packet, sensor fault, absent device, USB
error, unsupported device, timeout, and busy device are different outcomes.
They guide application policy rather than merely serving as debug numbers.

## The opaque handle hides the implementation

```cpp
typedef struct st_ThermoDevice ThermoHandle;
```

The header declares the type without showing its fields. Clients may hold a
pointer to it but cannot access its layout. The implementation defines the
object with a Windows handle, WinUSB interface handle, discovered pipe
addresses, and persistent response parser.

Opaque does not mean magically safe. A random pointer or a pointer used after
close is still invalid. The code checks for null, not for arbitrary pointer
authenticity. Callers must retain and use the live handle according to the
documented lifetime contract.

## Opening publishes only complete state

`Thermo_Open` validates the output-pointer argument, sets the output slot to
null, and allocates private state using `new (std::nothrow)`. That form reports
allocation failure through a null pointer rather than an allocation exception.
It then calls the internal USB opener.

If initialization fails midway, `Thermo_Close` releases whatever resources were
already acquired. If it succeeds, only then does `*ppHandle = pDevice` publish
the object to the client. The client therefore receives either a usable handle
or a null slot with an error status, not a half-open object it must inspect.

The documented requirement to pass an empty output slot matters. The function
clears that slot; passing storage containing your only reference to another
live handle would lose that reference and leak the earlier object. Input
validation cannot substitute for ownership discipline.

## Cleanup is deliberately ordered

`Thermo_Close` first frees the WinUSB interface, then closes the underlying
Windows device handle, then deletes the object. A null handle is harmless.
Closing the same nonnull pointer twice is not harmless because the first call
has already deleted its object.

The client must close the device before `FreeLibrary`. Otherwise the code that
knows how to release private resources may no longer be loaded. Function
pointers from an unloaded module must not be called. Microsoft's
[runtime-linking documentation](https://learn.microsoft.com/en-us/windows/win32/dlls/run-time-dynamic-linking)
explains module loading and reference lifetimes; the device-object ordering
here is an additional project-specific contract.

## Threading is an explicit limitation

The header requires serialized operations on each handle and forbids closing
during a read. Why? The parser buffer is mutable, pipe timeout policies are
changed per read, and request/response state is not protected by a per-handle
mutex. Two simultaneous calls could interfere even though each generates a
different token.

The exclusive Windows open prevents another ordinary client from sharing this
device through that handle-opening policy. It does not serialize two threads
inside the same process using the same `ThermoHandle`. Interprocess exclusion
and in-process synchronization solve different problems.

## API versus ABI

An API describes what callers may do and what results mean. An ABI describes
binary details such as symbol naming, argument passing, type sizes, and
layout. Keeping exported signatures stable is useful, but changing semantics
from queued samples to fresh conversions still matters to users. That is why
the DLL API version is checked by the console.

The public handle avoids exposing STL containers across the boundary. Memory
allocated for private state is freed inside the same library that allocated
it. This keeps ownership simple and avoids forcing clients to reproduce its
allocator and implementation choices.

## Check your understanding

1. Why does the function return status rather than temperature directly?
2. What does the opaque type hide, and what misuse does it fail to prevent?
3. Why must close occur before DLL unload?
4. Why is exclusive device opening insufficient for thread safety?

Answers: [Chapter 13](24-answers.md#chapter-13).

<!-- NAVIGATION -->

---

[Previous: 12. Control requests and the Microsoft descriptors](12-control-and-os-descriptors.md) · [Contents](README.md) · [Next: 14. Discovering and opening the Windows device](14-windows-device-discovery.md)
