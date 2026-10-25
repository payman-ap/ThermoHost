# ThermoHost: roadmap after the fresh-read snapshot

Updated: 2026-10-26.

## Purpose and priorities

Continue learning how to design, build, load, test, and distribute a native
Windows DLL. Use the thermometer as a concrete device behind the API. Keep the
console as the primary client until the DLL contract and lifecycle are clear;
Qt comes afterward as another consumer of that same interface.

Recommended order: finish the current checkpoint, document calling the DLL,
introduce continuous acquisition with latest-sample delivery, strengthen tests
and lifecycle handling, package a small SDK, then add another client.

This is a plan, not a description of features already implemented. In
particular, background streaming, a cached latest-sample API, automatic
reconnection, and the proposed exports below do not exist yet.

## Starting point

| Item | Current state |
| --- | --- |
| Development branch | `main` |
| Snapshot commit | `0c63a34` |
| Snapshot branch | `fresh-read-snapshot` |
| Annotated tag | `fresh-read-request-response-v1` |
| Windows host | VS2019, MSVC v142, C++17, x64 |
| Firmware | Pico SDK 2.3.1, `thermometer_usb` target |
| DLL / wire versions | API 3 / protocol 2; these are separate version numbers |
| Acquisition | One new conversion requested by each `Thermo_ReadTemperature` call |
| Console | Continuous until Ctrl+C, or a finite `--samples N` run |
| Board evidence | Flashed successfully; three-read runs passed with 3 s and 20 s pauses |
| Remaining acceptance | Fault/recovery and Ctrl+C behavior on this version |

The current firmware waits 250 ms after a request before reading the MAX6675.
The sensor's specified conversion time is typically 170 ms, at most 220 ms;
250 ms is the chosen margin. Four new readings per second is a reasonable
starting acquisition rate. Faster USB delivery or application polling does
not make the sensor generate new measurements faster. See the
[MAX6675 datasheet, pages 2 and 5](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX6675.pdf).

The earlier streaming baseline is preserved by tag `live-thermometer-winusb`;
the earlier packet experiments are preserved by `packet-decoder-snapshot`
and branch `packet-playground`.

## Milestone 1: close the current learning checkpoint

Keep this milestone small; do not redesign streaming at the same time.

1. Run the current console continuously, check normal temperatures, and stop
   with Ctrl+C. Confirm the device closes and the process exits normally.
2. Test an open thermocouple and normal recovery using the established safe
   wiring procedure. Confirm the current version reports the fault without
   first returning the old stream of queued valid readings.
3. Retest a finite run. Remember `--samples 3 --interval-ms 20000` means three
   attempts total with two 20-second pauses, not three attempts every 20 s.
4. Record the actual commands, observations, and any limitations in the
   existing verification notes. Mark the initial DLL/WinUSB learning phase
   complete only after its remaining checks pass.

Done when: the current snapshot has a clear acceptance record and its
request/response semantics are understood. Preserve its tag even when later
streaming work becomes the preferred implementation.

## Milestone 2: write a practical DLL caller guide

This should precede adding more exported functions. Existing `dll_basics.md`
is primarily an exercise history, not a complete current client manual.

Document one minimal working explicit-loading example using the public
header, `LoadLibraryW`, checked `GetProcAddress` calls, and correctly typed
function pointers. The lifecycle is load, resolve, check API version, open,
read, close, then unload. Show output parameters and status checks explicitly.
Microsoft's [runtime linking example](https://learn.microsoft.com/en-us/windows/win32/dlls/using-run-time-dynamic-linking)
explains why this approach does not require an import library.

Include:

- Current exports, arguments, statuses 0–9, and output-on-failure behavior.
- Opaque handles: allocated and freed inside the DLL; never delete them in
  the client or call through function pointers after unloading.
- x64 client/DLL compatibility and Debug versus Release dependencies.
- The different roles of the public `.h`, runtime `.dll`, and import `.lib`.
- A separate short example of implicit linking for comparison.
- How to build a client outside this solution and where to place its DLL.
- Current threading rules: serialize calls on a handle and never close it
  while a read is active.

Done when: a new console project outside ThermoHost can load the DLL and
read a measurement by following only the guide and supplied header.

## Milestone 3: continuous acquisition and latest-sample delivery

This is the recommended next implementation project. Create a working branch
such as `feature/latest-sample-streaming` from current `main`; keep the
request/response snapshot intact.

### Define the behavior before changing code

There are three different useful operations:

| Operation | Meaning |
| --- | --- |
| Existing fresh read | Request a conversion started after the call; wait for its response |
| Latest-sample lookup | Return the newest received sample immediately, together with freshness information |
| Wait for next sample | Block until a sample newer than a supplied sequence arrives, or time out |

Keep these meanings distinct. Preserve `Thermo_ReadTemperature` for the
existing request/response behavior; add explicitly named streaming functions
instead of silently changing that export's contract. Suggested names, subject
to API review, are `Thermo_StartStreaming`, `Thermo_GetLatestSample`,
`Thermo_WaitForNextSample`, and `Thermo_StopStreaming`.

Initially support one mode at a time per handle. Reject a request/response
read during streaming rather than allowing two consumers to race for bulk IN.
Negotiate firmware capabilities and a revised streaming protocol explicitly;
do not reinterpret current 32-byte responses as stream messages.

### Implement incrementally

1. Define a stream frame with an explicit version/type, sequence number,
   device-session identifier, device timestamp, temperature, and sensor flags.
   A sequence identifies a measurement even when its temperature is unchanged.
2. Let firmware complete conversions at approximately 4 Hz and send stream
   frames while streaming is enabled. Keep USB servicing responsive. Start
   and stop must have defined acknowledgement and queued-data handling.
3. Give each streaming DLL handle one receive worker. It continuously parses
   frames and atomically replaces a bounded latest-sample record. A slow
   application must not cause an ever-growing queue inside the DLL.
4. Store the host monotonic receive time with each record. Expose sample
   sequence, sensor status, and age, not just a temperature value.
5. Add immediate latest-sample lookup, then an optional wait-for-next operation
   with a caller-selected timeout. Protect shared state and notification with
   appropriate synchronization; avoid polling loops that consume a CPU core.
6. Test the stream first with a console that deliberately reads slowly.
   Sequences should jump forward, and acquisition must continue independently.

Distinguish no sample yet, repeated lookup of the same sample, stale data,
sensor fault, and disconnected transport. A fault must replace the previous
valid state immediately on receipt, rather than leaving an old good value
appearing current. Define whether a latest-sample function returns a snapshot
containing a sensor fault or an operation error, and document that choice.

Host receive age measures time since receipt, not exact physical measurement
age. Buffered packets can arrive recently while describing older conversions.
Use sequence/session information and device timestamps to detect backlog,
restart, and continuity; handle timestamp wraparound. Do not subtract device
uptime directly from a Windows clock. An exact cross-device age guarantee
requires an additional clock-mapping or synchronization design.

Done when: slow console polling has bounded storage and returns the newest
available state rather than replaying every old reading; faults and connection
state are visible; the age definition and first-sample behavior are documented.

## Milestone 4: dependable DLL lifecycle and diagnostics

Implement this alongside the streaming worker rather than postponing shutdown
correctness until after a UI exists.

- Start simple with bounded blocking reads in the worker. Profile before
  adding multiple outstanding transfers or more threads.
- Stop acquisition, wake or cancel pending operations as appropriate, wait
  for I/O completion and worker exit, then free WinUSB and Windows resources.
  Never unload the DLL while its worker is executing.
- If overlapped I/O is introduced, keep buffers and `OVERLAPPED` storage alive
  until completion. A cancellation request alone does not mean I/O has ended.
  See [WinUsb_ReadPipe](https://learn.microsoft.com/en-us/windows/win32/api/winusb/nf-winusb-winusb_readpipe)
  and [Windows I/O cancellation](https://learn.microsoft.com/en-us/windows/win32/fileio/canceling-pending-i-o-operations).
- Make stop safe when already stopped; specify what close does to an active
  stream. State which public operations may run concurrently.
- Expose readable status descriptions and useful per-handle transport error
  details, with documented ownership and lifetime of returned information.
- Detect unplugging and invalidate current-data claims promptly. First require
  explicit close/reopen; add automatic reconnect only as a later, separate feature.

Done when: repeated start/stop/open/close cycles, unplug during a wait, and
Ctrl+C during acquisition do not hang, leak resources, or call unloaded code.

## Milestone 5: tests and a small distributable SDK

Extend the existing parser tests around meaningful failure boundaries:
fragmented and combined frames, invalid sizes/flags/versions, dropped data,
sequence gaps/wraparound, session restart, stale tokens, faults, and unchanged
outputs where the API promises them.

Add a fake transport so lifecycle, timeout, cache replacement, and shutdown
can be tested without a connected board. Keep separate hardware smoke tests
for discovery, real acquisition, faults, and unplug/replug; a fake does not
prove driver binding or sensor behavior.

For the public ABI, keep C linkage, explicit calling conventions, opaque
handles, and documented ownership. Avoid STL or exceptions crossing the DLL
boundary. Any public sample structure needs documented field types, layout,
size/version negotiation, and compatibility behavior; fixed-width fields alone
do not eliminate alignment differences.

Package a Release x64 DLL, public headers, caller example, API guide, and the
import library for clients choosing implicit linking. Document the required
MSVC runtime. Keep the explicit-loading example free of a required import
library dependency. Add Windows CI for hardware-independent builds/tests when
the local commands are stable; use firmware CI separately if worthwhile.

Done when: a clean external client can consume the package without depending
on the original solution's build order or machine-specific paths.

## Milestone 6: a second client, then Qt

Choose a small second client as a DLL portability exercise: another native
console, Python through `ctypes`, or C# through P/Invoke. One is enough initially.
Verify bitness, calling convention, structure layout if used, and ownership.

Then build a minimal Qt app that loads the same DLL and displays temperature,
fault/connection status, and sample age. Keep WinUSB and packet parsing inside
the DLL. Use a UI timer for a nonblocking latest-sample lookup, or a client
worker for blocking operations; do not block the UI thread on sensor conversion.
Add plotting only after start/stop and close behavior are reliable.

Done when: the same DLL serves console and GUI clients without duplicated
transport logic. Qt integration is a later milestone, not a prerequisite for
the initial DLL learning phase.

## Optional directions after the core is stable

| Direction | Learning goal |
| --- | --- |
| Synthetic high-rate source | Measure USB/parser throughput independently of the slow temperature converter; track loss and latency |
| Lossless recording | Design a bounded FIFO, overflow reporting, and disk logging separately from latest-value monitoring |
| Multiple boards | Enumerate devices and select by serial number rather than always opening the first |
| Reconnection | Define new device sessions and avoid reusing cached data from a disconnected session |
| STM32 port | Reuse the host API and negotiated wire protocol while replacing MCU-specific acquisition and USB code |
| Faster acquisition hardware | Choose a converter based on a concrete target sample rate, resolution, noise, and interface requirement |

Sending duplicates more often is useful for transport experiments only when
clearly labeled; it is not a higher rate of independent sensor measurements.

## Working and snapshot practice

Use one focused learning change at a time. Before each API change, write down
the caller-visible contract; afterward, build, run relevant tests, and record
the observed result. Commit at working checkpoints with a concise explanation
of behavior and validation. Merge a feature branch into `main` after its
acceptance checks pass, then tag meaningful milestones rather than every edit.

Tags preserve exact checkpoints; experiment branches may continue evolving.
For example, an independent checkout of the current snapshot is:

```powershell
git clone --branch fresh-read-snapshot https://github.com/payman-ap/ThermoHost.git ThermoHost-fresh-read
```

To start an experiment from its fixed tag in that clone:

```powershell
git switch -c my-fresh-read-experiment fresh-read-request-response-v1
```

Use a separate directory for experiments if you want the working `main`
checkout and its build outputs undisturbed. Never move the existing snapshot
tag to a later commit.

The immediate next action is milestone 1's remaining board checks, followed by
the caller guide. The next substantial implementation is milestone 3's
continuous acquisition and latest-sample delivery.
