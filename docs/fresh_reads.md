# Fresh reads and initial-phase acceptance

## What changed

The earlier 20 samples were a finite demonstration loop, not a sensor or USB
limit. The console now runs continuously until Ctrl+C. `--samples N` limits
read attempts (including fault results), and `--interval-ms N` adds an idle
pause after each result. A sensor fault is printed and reading continues, so
reconnecting a probe can recover within the same session. USB errors and
timeouts end the session with a nonzero process exit code.

Each DLL read now sends a request. The board restarts a conversion, waits
250 ms, and replies with that request's token. There is no idle stream to
build up old readings. The DLL rejects replies belonging to previous reads,
including late replies after timeouts. See [wire protocol v2](protocol.md).

The normal console now uses only the DLL for USB access. Earlier discovery,
simulated values, and null-argument demonstrations remain in Git history.
`Thermo_GetSimulatedTemperature` remains exported for compatibility but is
not a source of readings in this console.

## Build and run

Build the Visual Studio solution as Debug x64. From a Pico SDK terminal in
the `firmware` folder, build `cmake --build build --target thermometer_usb`.
Flash `firmware/build/thermometer_usb.uf2` using BOOT mode. The previous firmware
will produce UnsupportedDevice (7) with the updated DLL; this is intentional.
No driver change is required because the USB interface descriptors are unchanged.

From `dll/ThermoHost/x64/Debug`:

```powershell
.\ThermoConsole.exe --samples 5
.\ThermoConsole.exe --samples 3 --interval-ms 3000
.\ThermoConsole.exe
```

The first command checks normal reads. The second deliberately leaves the
board idle between reads: change the probe temperature during a pause and
confirm the next result reflects a new conversion. All reads, including the
first, should take about 250 ms plus transport/scheduling overhead. The extra
3000 ms pause is not included in each printed read time.

The last command runs until Ctrl+C and then prints `Device closed ...`.
Ctrl+C waits for an active bounded read to finish before cleanup.

## Local verification

- Debug x64 DLL and console build succeeded.
- Pico SDK 2.3.1 live firmware build succeeded.
- `tests/run_fresh_response_tests.cmd` passed: stale token rejection,
  fragmented frames, resynchronization, negative/positive decoding, fault and
  invalid flag handling, and partial old response followed by a new request.
- CLI help and invalid numeric arguments were checked.
- The connected board with previous firmware was correctly rejected with
  UnsupportedDevice (7), rather than returning queued readings as fresh.

The test script currently locates the installed VS2019 Professional tools;
adjust its vcvars64 path if using a different Visual Studio installation.

## On-board verification

The user flashed the updated firmware and supplied successful API-version-3
runs with `--samples 3 --interval-ms 3000` and `--samples 3 --interval-ms 20000`.
Each run returned exactly three successful measurements and closed normally.
Reported read times were 234–250 ms; the 20-second-pause run included a change
from about 23 C to 52.5 C. These confirm finite request/response operation and
successful reads after idle pauses. Fault/recovery and Ctrl+C tests for this
version have not yet been confirmed.

`--interval-ms` pauses after each result, excluding the final result of a
finite run. It is not included in the printed read time. Thus three reads
with 20000 ms pauses take approximately 40.75 seconds overall.

## Snapshot

Tag `fresh-read-request-response-v1` and branch `fresh-read-snapshot` preserve
this implementation before any return to continuous streaming. Acquisition
is one conversion per request; this snapshot does not implement background
streaming or immediate retrieval of a cached latest sample.

## Initial phase

Previously verified: DLL creation, explicit loading and missing-export errors,
opaque handle ownership, argument validation, packet decoding, WinUSB driver
binding, live MAX6675 acquisition, and sensor-fault reporting.

Implementation and local checks for freshness are complete. Before calling the
initial phase finished, verify on the newly flashed board:

- Every normal read is a new conversion, including after idle pauses.
- A disconnected probe produces SensorFault without old valid readings first;
  restoring the connection produces normal readings again.
- Ctrl+C closes cleanly; a finite run exits after its requested attempt count.

On-board freshness acceptance is partially complete; the remaining fault/recovery
and Ctrl+C checks above are pending. Calibration, automatic hot-plug
recovery, multiple-device selection, streaming throughput, and Qt are later
work, not initial-phase completion requirements.
