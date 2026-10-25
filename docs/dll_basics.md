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
- Current console writes its function-pointer type directly. A shared
  declaration can instead supply it with `decltype(&Thermo_GetApiVersion)`;
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

## Next

Practice status codes and output parameters using a simulated temperature,
then introduce an opaque device handle before implementing WinUSB.
Qt is deferred at the user's request.

## References

- https://learn.microsoft.com/en-us/cpp/cpp/dllexport-dllimport
- https://learn.microsoft.com/en-us/windows/win32/dlls/using-run-time-dynamic-linking
- https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes--0-499-
