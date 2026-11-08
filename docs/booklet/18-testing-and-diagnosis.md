# 18. Testing the boundaries and diagnosing failures

## Start with the claim you want to establish

“It works” is too broad to be a useful test result. A parser test establishes
decoding behavior for supplied bytes. A descriptor inspection establishes what
the connected device advertises. A temperature-change test establishes a
measurement path under the tested conditions. Each result has a scope.

The project records earlier stages in session notes and fresh-read evidence in
[fresh_reads.md](../fresh_reads.md). That record reports successful finite runs
after long pauses, but marks fresh-version fault/recovery and Ctrl+C hardware
checks as unconfirmed. The booklet does not turn documented intentions into
newly observed results.

## Host-only parser tests

From the repository root:

```powershell
.\tests\run_fresh_response_tests.cmd
```

The script calls a VS2019 Professional environment setup batch file, compiles
the C++ test into `build/tests/`, then runs it. If your installation differs,
adjust the explicit tool path in your experimental copy. Its assertions are
active in this script's normal build because it does not define `NDEBUG`.

Study an assertion as a contract. After an old token, output must remain -999.
Before the final byte of a current frame, completion must remain false. After
the final byte, status and value must match. These are meaningful externally
observable parser properties rather than tests that merely duplicate every
line of implementation.

## A layered diagnostic table

| Observation | First boundary to inspect | Useful evidence |
| --- | --- | --- |
| Cannot load DLL | Executable/module environment | Full error code, output files, architecture. |
| No matching device | Attach/enumeration/binding/GUID | Device Manager properties, firmware target. |
| Unsupported firmware | Interface or capability validation | Endpoints, control reply, flashed image. |
| Device busy | Exclusive open | Other instances or tools with open sessions. |
| Timeout after successful open | Request/response path | OUT command, firmware activity, IN response. |
| SensorFault response | Sensor/wiring | Raw fault bit, probe connection, valid USB response. |
| Valid but implausible temperature | Measurement/decoding | Raw SPI word, expected scaling, physical setup. |

Do not begin every investigation by reinstalling drivers. If a current
sensor-fault response is decoded, important USB operations have already
succeeded. Rebinding Windows will not reconnect the thermocouple.

## Inspect the actual device

In Device Manager, inspect hardware IDs, driver information, and device
interface-related properties where available. Record the VID/PID, product
name, and whether WinUSB is serving the runtime vendor device. The BOOT-mode
drive and runtime vendor device are different observations.

USB descriptor inspection tools can help verify interface count, class,
endpoint addresses, and packet sizes. A USB software capture can reveal
submitted transfers and their payloads. Such tools observe specific layers;
they do not automatically expose sensor timing or every electrical bus event.
The booklet does not install a capture filter or change driver bindings as
part of its exercises.

## A controlled hardware acceptance sequence

Build and flash the current live firmware, then build the current x64 host.
Run three attempts without pauses and save the output. Run three with a
multi-second pause. Change the probe temperature during a pause and observe
the next result. This probes the no-idle-backlog policy.

Next verify fault/recovery in a controlled low-voltage setup, following the
project's disconnected-USB wiring practice. After restoring a known good
connection and starting a session, establish how your chosen safe fault setup
will be exercised. Record statuses and whether valid output resumes. Do not
claim an already completed current-version fault test merely because older
streaming notes contain one.

Finally test Ctrl+C during normal continuous reading and during a deliberately
long idle pause. Confirm cleanup output and that another process can open the
board afterward. An atomic stop flag is implementation evidence; a successful
hardware/session test is behavioral evidence.

## Distinguish disappearance from silence

Unplugging the device and leaving a configured device silent are different
faults. Disappearance usually produces transport errors or absent-device state.
Silence exercises timeout handling. Wrong tokens exercise stale-response
rejection. Invalid reserved bits exercise content validation after identity
matching. One negative test cannot stand in for all four.

The historical silent probe is useful with historical host code, but the
current capability gate stops that test at open. For current timeout testing,
use a clearly labeled experimental firmware that advertises the current
capability but deliberately withholds a reply. Restore the normal image after
the experiment. Do not change production behavior just to obtain one status.

## Timing measurements need provenance

Record configuration, host load, hub topology, attempt count, and what interval
you measured. A console elapsed time is host-observed call duration. A logic
analyzer interval between CS edges is device-side activity. They are related
but not identical.

Use many observations for timing distributions, not just a single fastest
read. Report typical, maximum observed, and relevant percentiles together with
test conditions. “Maximum observed” is not a proven worst-case bound. A rare
deadline miss may not appear in a short run.

## Check your understanding

1. What does the parser test fail to establish about the physical sensor?
2. How would you distinguish a timeout test from a disconnect test?
3. Why should a captured correct response token be associated with its OUT
   request rather than interpreted in isolation?
4. Which current-version board checks remain unconfirmed in the repository record?

Answers: [Chapter 18](24-answers.md#chapter-18).

<!-- NAVIGATION -->

---

[Previous: 17. The console: loading, looping, and stopping](17-console-loading-and-lifetime.md) · [Contents](README.md) · [Next: 19. Real-time thinking: latency, jitter, freshness, and queues](19-real-time-and-buffering.md)
