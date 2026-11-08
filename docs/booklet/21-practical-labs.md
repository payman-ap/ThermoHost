# 21. Practical labs: turn explanations into evidence

These labs are ordered. Complete the observation and record the result before
adding another change. Use a separate experimental branch for modifications,
retain the working firmware and host binaries, and restore the current live
image when finished. Suggested modifications are assignments, not features
already implemented by this booklet.

## Lab 1 — Map one successful call

**Goal:** distinguish responsibilities without changing code.

Open the console, DLL implementation, live firmware, and parser. Find the
console's `pRead` call, the DLL's request construction, firmware command
completion, packet encoding, and parser publication. Write a table naming the
processor, input representation, and output representation at each boundary.

**Expected evidence:** a trace from a `double*` on Windows through twenty
command bytes and thirty-two response bytes to a raw sixteen-bit SPI word.
Explain why the order of that written trace differs from the chronological
order of the physical measurement if you start from the displayed result.

**Extension:** identify which pointers refer to local arrays and which opaque
handles represent longer-lived resources. None of their numeric pointer
values should be part of your wire trace.

## Lab 2 — Decode a measurement on paper

**Goal:** make units and ordering automatic.

Start with raw word `0x0330`. Expand enough bits to locate bit 2 and bits 14–3.
Compute counts, Celsius, temperature x100, and the four little-endian bytes.
Use uptime 1000 and the teaching token `00` through `0F` to construct a full
response. Compare your result with chapter 11.

**Expected evidence:** 102 counts, 25.50 C, 2550 hundredths, `F6 09 00 00`,
and a thirty-two-byte response. Now set bit 2 and explain why the displayed
temperature must disappear even though temperature bits still exist.

**Extension:** encode synthetic -10.25 C, then explain why that is a decoder
exercise rather than a MAX6675 hardware requirement.

## Lab 3 — Build without conflating platforms

**Goal:** identify each toolchain and artifact.

Build `thermometer_usb` in the configured Pico terminal and Debug x64 in Visual
Studio. Record the commands or GUI configuration, output paths, and target
names. Run `ThermoConsole.exe --help` without opening hardware.

**Expected evidence:** a UF2 for the board and an EXE/DLL pair for the PC.
Explain the `.elf`, `.bin`, `.lib`, and `.pdb` artifacts you encounter, without
assuming they are all installed on the microcontroller.

**Extension:** inspect the CMake compile definition enabling the capability
response. Predict what compiling the shared descriptor file without it does.

## Lab 4 — Inspect identity before reading temperature

**Goal:** connect source descriptors to observed device properties.

With the live image running, inspect the runtime device's properties in Device
Manager. Record VID/PID, visible name, and driver information. Compare with
`usb_descriptors.c` and `winusb_descriptors.c`. If a descriptor viewer is already
available, inspect interface class, endpoints, and packet size as well.

**Expected evidence:** a vendor device using WinUSB and the intended endpoint
layout. Absence of a COM port is expected for this target. Do not use the
BOOT-mode drive's properties as the runtime result.

**Extension:** explain why the current product string cannot distinguish live
firmware from the probe. Name the application-level query that can.

## Lab 5 — Exercise the host-only parser

**Goal:** observe success, stale data, faults, and fragmentation independently
of USB hardware.

Run `tests/run_fresh_response_tests.cmd`. Read each assertion and describe its
claim in plain language. In your experimental branch, add a test case feeding
three noise bytes followed by a valid current response, and another with a
nonzero reserved byte at offset 5. Preserve the real API's output-on-failure
contract in the test expectations.

**Expected evidence:** no publication while bytes are incomplete or tokens
are wrong; InvalidPacket for the matching reserved-byte case; unchanged output
on that failure. Feed arrays incrementally to model fragmentation; do not
assume a packet boundary resets parser state.

**Extension:** partition the same response at every possible split position.
The final result should be independent of how a transport chunks the bytes.
This tests a property stronger than one convenient example split.

## Lab 6 — Confirm current finite reads

**Goal:** establish a reproducible real-device baseline.

Flash the current `thermometer_usb.uf2` and build the current Windows pair.
From `ThermoHost/x64/Debug`, run:

```powershell
.\ThermoConsole.exe --samples 3
```

**Expected evidence:** API version 3, exactly three attempt lines, plausible
statuses, and cleanup output. Record actual timings rather than replacing
them with the book's illustrative values. If a stage fails, use chapter 18
before changing code.

**Extension:** run the finite command again. A successful second open after
the first completes is evidence that ordinary cleanup released exclusive
access in that scenario.

## Lab 7 — Test the idle-backlog hypothesis

**Goal:** distinguish requested acquisition from queued streaming.

Run three reads with a 3000 ms pause, then with 20000 ms. Change the probe's
temperature during a pause using a controlled setup appropriate to your
hardware. Keep the setup within ordinary safe operating conditions; a modest
change is enough to illustrate the software behavior.

**Expected evidence:** each next attempt spends approximately a conversion
interval and reflects a new conversion, rather than returning a long sequence
of pre-pause queued samples. Three attempts with twenty-second pauses take
about 40.75 seconds plus overhead, not sixty seconds.

**Limit of the evidence:** this does not establish exact sample age, calibration,
or a worst-case bound. It is an acceptance observation consistent with the
request/response design. The code's CS restart ordering supplies the causal
implementation argument.

## Lab 8 — Observe SPI timing

**Goal:** compare conversion waiting with the short serial read.

If you have a suitable logic analyzer, observe CS, SCK, and SO with a common
ground and appropriate input levels. Trigger around a read attempt. Identify
the restart pulse and later sixteen-clock read. Decode one word manually and
compare with the host temperature if your capture permits reliable pairing.

**Expected evidence:** approximately 250 ms between conversion restart and
readout, with the SPI clock burst much shorter. Mark which timings are
measured, which are configured, and which remain uncertain.

**Extension:** if the analyzer supports protocol decode, change only the
decoder's phase setting and observe how an incorrect tool configuration can
misrepresent an unchanged waveform. Restore the matching setting afterward.

## Lab 9 — Test stop and ownership

**Goal:** verify cleanup without racing an active read.

Run continuous mode, request Ctrl+C, and observe session termination. Repeat
while a long interval pause is active. Start another finite run afterward.
Record whether cleanup was printed and whether the board reopened.

**Expected evidence:** the handler requests stop, main eventually closes the
device, and a subsequent process can open it. An active read may finish before
the stop is acted upon. Do not call the close function from a debugger on
another thread to simulate cancellation; that violates the current contract.

**Extension:** explain the difference between stopping during a pause and
stopping during a synchronous USB read. Identify the point at which main
checks the atomic flag in each case.

## Lab 10 — Separate silence, malformed data, and absence

**Goal:** construct different negative tests deliberately.

Use host-only tests for wrong token and reserved-bit failures. For a hardware
timeout test, create a clearly labeled experimental live firmware that keeps
the capability reply but withholds the measurement response. A current host
should open successfully and later time out. Restore the normal firmware.
Separately observe unplugging during a session and record the resulting
transport outcome.

**Expected evidence:** wrong tokens are ignored; matching malformed flags
produce InvalidPacket; silent current firmware produces a waiting failure;
disappearance is diagnosed as a transport/device event. Exact Windows errors
can depend on the point of removal, so record rather than invent them.

**Extension:** explain why `SEND_TEST_PACKETS = 0` in the old probe is not a
current-host timeout test. Its missing capability stops the session earlier.

## Lab 11 — Explore timing as a distribution

**Goal:** stop treating a single elapsed number as a guarantee.

Collect a sufficiently long finite run for your study, preserving output in
a text file. Compare an otherwise idle host with a realistic workload. Compute
minimum, median, upper percentiles, and maximum observed read duration. State
sample count and test conditions alongside the values.

**Expected evidence:** a distribution of host-observed durations. No graph in
this booklet supplies that distribution; you must collect it. A maximum from
a finite run is an observed value, not a proven upper bound.

**Extension:** write a proposed display-age requirement and decide whether
temperature-only API results contain enough timing information to verify it.

## Lab 12 — Design the next protocol on paper

**Goal:** connect this exercise to future DAQ and driver work.

Specify a hypothetical stream with sequence number, timestamp definition,
sample count, overflow indicator, and payload size. Choose whether to preserve
every sample or keep the latest. State behavior at reconnect and during
overload. Design host parser tests before implementing transport changes.

**Expected evidence:** a byte layout and state diagram that explain framing,
version negotiation, units, clock meaning, and loss. Explain why a 64-byte USB
packet limit does not require every application frame to be exactly 64 bytes.

**Extension:** map the design to an asynchronous host reader or future WDF
sample conceptually. Define buffer ownership through completion and
cancellation. Do not begin by replacing the working Windows driver.

<!-- NAVIGATION -->

---

[Previous: 20. From this project toward device-driver development](20-toward-device-drivers.md) · [Contents](README.md) · [Next: 22. Glossary and quick reference](22-glossary-and-reference-tables.md)
