# 17. The console: loading, looping, and stopping

## Command-line validation happens first

[ThermoConsole.cpp](../../ThermoHost/ThermoConsole/ThermoConsole.cpp) accepts
`--help`, `--samples N`, and `--interval-ms N`. It parses unsigned numbers using
`std::from_chars`, requires complete consumption of the value, and checks
allowed ranges. `--samples` must be positive; interval may be zero through
60000 ms. With no sample option, the internal zero means unlimited attempts.

That distinction explains an apparent contradiction: zero is the internal
default for unlimited reading, but `--samples 0` is rejected. An external
interface need not expose every internal sentinel.

Validating before loading or opening devices makes malformed command lines
cheap to diagnose. `--help` can work on a computer without the board. The
program does not need a USB session to explain its own usage.

## Runtime loading is explicit

```cpp
HMODULE Module = LoadLibraryW(L"ThermoDevice.dll");
```

The module handle represents code loaded into the process. The console then
looks up four names: version, open, read, and close. Each address is cast to a
function-pointer type obtained using `decltype(&Thermo_...)`, keeping the type
consistent with the header's declaration.

Checking all four pointers before use prevents a missing export from becoming
a null function call. Checking API version 3 prevents a loaded older DLL from
silently providing the previous measurement semantics. A successfully loaded
module is not automatically a compatible library.

Microsoft's [explicit-linking description](https://learn.microsoft.com/en-us/cpp/build/linking-an-executable-to-a-dll?view=msvc-170)
and [runtime-loading description](https://learn.microsoft.com/en-us/windows/win32/dlls/run-time-dynamic-linking)
provide background. The exact exports and version policy here are specific to
ThermoHost. A production launcher could choose a more deliberate DLL path;
this exercise loads by filename and uses normal Windows search behavior.

## Open once, read repeatedly, close once

After resolving symbols and checking the version, the console calls `pOpen`.
Failure prints a named status and exits after unloading the module. Success
creates one session with one opaque device handle.

The loop initializes a local temperature, records start time, calls the DLL,
increments the attempt counter, and prints the result. It prints temperature
only for Ok. SensorFault is visible but recoverable, so another attempt is
allowed. Other statuses end the session with a nonzero process exit code.

`--samples 3` means three attempts including faults, not three successful
temperatures. A run of fault responses can therefore finish normally after
three attempts while reporting no valid temperatures. Process success and
physical measurement validity should not be confused.

## A pause is not a sample period

After each nonfinal result, the console optionally pauses. It uses short
`Sleep(10)` calls while checking stop state, rather than one uninterruptible
multi-second sleep. The extra interval is measured from the start of the pause,
not from conversion start.

For three normal reads with `--interval-ms 3000`, total duration is approximately
`3 * 250 ms + 2 * 3000 ms`, plus overhead: about 6.75 seconds. There are only
two pauses because the loop exits immediately after its final requested
attempt. The printed elapsed value covers each DLL read, not these pauses.

If a user wants exactly one sample per second, adding a 1000 ms pause is not
the correct scheduling formula: each read already consumes time. A periodic
acquisition design needs explicit target deadlines and a policy for late
iterations. This console is a demonstration reader.

## Ctrl+C uses a shared atomic flag

The console control handler accepts Ctrl+C and Ctrl+Break, stores true in
`StopRequested`, and returns. It does not call `Thermo_Close` from the handler.
The main thread checks the flag between operations and owns cleanup.

This avoids closing a handle while the main thread is still using it in a
read. The atomic variable makes communication of the stop request safe between
the handler's execution context and the main loop. It does not make the DLL's
entire mutable device object thread-safe.

An active synchronous read is not canceled immediately by this flag. It must
complete or fail before main observes the next loop condition and cleans up.
The intended request/response budget limits normal waiting policy, but this is
not a hard real-time stop guarantee.

## Cleanup order is visible

```cpp
pClose(pDevice);
SetConsoleCtrlHandler(HandleConsoleSignal, FALSE);
FreeLibrary(Module);
```

The device closes while its implementation is still loaded. The handler is
removed and the DLL module is released. Every early return after successful
open must preserve the same ownership logic. The handler-installation failure
path explicitly closes and unloads before returning.

The current code uses manual cleanup with clear paths. A larger application
could introduce RAII wrappers for module and device ownership to reduce the
number of paths requiring explicit cleanup. Such a refactor should preserve
the public DLL lifetime contract, not merely replace syntax.

## Diagnose the console's messages by stage

`LoadLibrary failed` concerns module loading. `Missing DLL export` concerns
symbol availability. `Open: Unsupported firmware` concerns device capability.
`Sample ... Read timed out` concerns a started session that did not produce a
current response in the waiting policy. The labels expose the operation that
failed and should determine your next check.

## Check your understanding

1. Why is `--samples 0` invalid although zero is the internal default?
2. Why should the Ctrl+C handler avoid closing the device?
3. Approximately how long do three reads with 20-second pauses take?
4. Why can a successful process exit coexist with three sensor faults?

Answers: [Chapter 17](24-answers.md#chapter-17).

<!-- NAVIGATION -->

---

[Previous: 16. Parsing fragmented and stale responses](16-parser-and-fragmentation.md) · [Contents](README.md) · [Next: 18. Testing the boundaries and diagnosing failures](18-testing-and-diagnosis.md)
