# DLL basics: verified exercises

## Setup

- Visual Studio 2019, MSVC v142, C++17, Debug x64.
- Solution: `dll/ThermoHost/ThermoHost.sln`.
- `ThermoDevice`: native Dynamic Library (.dll).
- `ThermoConsole`: native console Application (.exe).
- Shared output folder: `dll/ThermoHost/x64/Debug`.

## Exports and implicit linking

- Exported `Thermo_GetApiVersion` with C linkage and explicit `__cdecl`.
- Shared `ThermoDevice.h` chooses `dllexport` when `THERMODEVICE_BUILD`
  is defined and `dllimport` otherwise.
- DLL build produces a DLL, import LIB, and EXP file.
- Project reference links the console to the import library.
- The user confirmed console output `DLL API version: 1`.
- Changed the DLL's return value to 2, rebuilt only the DLL, and ran the
  existing EXE directly. The user confirmed version 2 without rebuilding
  the console. The function signature stayed unchanged.

## Explicit loading

- Console now uses `LoadLibraryW`, `GetProcAddress`, a typed function
  pointer, and `FreeLibrary`.
- Errors capture `GetLastError` before cleanup or output operations.
- Function pointers must match the exported signature and must not be used
  after releasing the DLL.
- The console initially wrote its function-pointer type directly. It now uses
  the shared declaration with `decltype(&Thermo_GetApiVersion)`;
  this unevaluated expression does not create an imported function call.
- A header alone does not create a runtime DLL dependency. A project
  reference was still present when last inspected, but the current program
  successfully starts without the DLL and reports its own load error.

## User-verified failure tests

1. Renamed the built DLL to `ThermoDevice.dll.disabled`, then ran the
   existing EXE without rebuilding: `LoadLibrary failed: 126`.
2. Restored the DLL filename: normal output returned.
3. Changed the lookup string to a nonexistent export and rebuilt the console:
   `GetProcAddress failed: 127`.
4. Restored the correct lookup string: normal version 2 output returned.

The code returns process exit status 1 for load failure and 2 for lookup
failure. The pasted outputs verified the printed Windows error codes;
process exit statuses were not separately shown.

## Status codes and output parameters

- Added `Thermo_GetSimulatedTemperature(double*)`, returning an integer
  status and writing 25.0 degrees C through the caller's output pointer.
- Null output pointers return `ThermoStatus_InvalidArgument` without a write.
- The console includes the shared header and uses `decltype` for its
  dynamically resolved function signatures.
- User-confirmed output: API version 2, read status 0, temperature 25 C,
  and null-pointer status 1.
- Source review confirmed export lookups are checked, Windows errors
  are saved before cleanup, and all calls precede `FreeLibrary`.

## Opaque handles and lifetime

- The public header aliases the forward-declared `st_ThermoDevice` as
  `ThermoHandle`; its fields are defined only in the DLL source.
- `Thermo_Open` allocates a simulated device in the DLL with
  `new (std::nothrow)` and returns it through a pointer-to-pointer.
- A null output slot pointer returns `ThermoStatus_InvalidArgument`.
  A valid output slot is cleared before allocation. Allocation failure has
  an explicit `ThermoStatus_OutOfMemory` path, but has not been forced in tests.
- `Thermo_Close` deletes the object in the DLL. Closing null is allowed;
  each non-null live handle must be closed exactly once before unloading.
- The caller clears its local handle after closing it. Other copies, if any,
  would still be dangling; arbitrary or previously closed handles are invalid.
- The console resolves and checks both lifecycle exports before opening.
- The user confirmed open status 0, a successful close, null-open status 1,
  and completion of a null-close call, alongside the earlier temperature tests.
- The original simulated-temperature helper remains independent of the handle.

## Handle-based temperature reads

- `Thermo_ReadTemperature` reads the temperature from the private device object.
- Null handles and null output pointers return invalid-argument status before
  accessing state or modifying output.
- During the first client test, the reads were placed after Close and after
  clearing the handle. The intended valid-handle read therefore returned status 1.
- Moved all handle reads before Close and added brief comments to the public
  header, DLL implementation, and console exercise blocks.
- Agent verification: built both projects with MSBuild for Debug x64, then ran
  the rebuilt console successfully (exit status 0).
- Confirmed null-handle status 1 with output still -999, live-handle status 0
  with temperature 25 C, and null-output status 1, followed by device cleanup.
- No reads are attempted on dangling non-null handles; those remain outside
  the API contract.

## Binary packet decoder checkpoint

- [Historical protocol specification](protocol_v1.md): 10-byte frames, little-endian signed
  32-bit hundredths of degrees C, fault flags, and a device timestamp.
- This corrects the roadmap's signed 16-bit hundredths field, which would
  overflow above 327.67 degrees C.
- The private decoder checks arguments, exact frame length, message type,
  reserved flag bits, and the sensor fault bit before writing output.
- Unsigned shifts assemble the four temperature bytes; a 64-bit intermediate
  handles the signed two's-complement interpretation without an out-of-range cast.
- Timestamp bytes are defined but not yet decoded or exposed by the DLL API.
- `Thermo_ReadTemperature` decodes the simulated packet in the private device
  object; the older fixed-value helper still returns 25 C independently.

User-verified exercises:

| Input | Status | Output behavior |
| --- | --- | --- |
| Valid 25.50 C frame | 0 | Returns 25.5 C |
| Open-thermocouple flag | 4 | Preserves -999 sentinel |
| Unknown message type 0x02 | 3 | Preserves -999 sentinel |
| Reserved flag bit 0x02 | 3 | Preserves -999 sentinel |
| Signed -10.25 C frame | 0 | Returns -10.25 C |
| Reported frame length 9 | 3 | Preserves -999 sentinel |

After testing, restored the valid 25.50 C packet and the full frame length;
the user confirmed normal results. Existing null-argument and cleanup exercises
also continued to pass.

This checkpoint is the starting point for branch `packet-playground` and tag
`packet-decoder-snapshot`. The branch can evolve with packet experiments;
the tag identifies the original tested source snapshot.

## Next

Continue toward firmware encoding and WinUSB on `main`, preserving the C API
and handle ownership contract.
Qt is deferred at the user's request.

## References

- https://learn.microsoft.com/en-us/cpp/cpp/dllexport-dllimport
- https://learn.microsoft.com/en-us/windows/win32/dlls/using-run-time-dynamic-linking
- https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes--0-499-
