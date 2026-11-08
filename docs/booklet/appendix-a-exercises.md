# Appendix A. Exercises and worked answers

Attempt each calculation or explanation before reading the answer. Problems
refer to concepts rather than requiring a connected board. The final experiments
apply those concepts to the repository.

## Chapter 1

**Problem.** Decode little-endian bytes `C4 09 00 00` as signed temperature
x100. Then interpret sensor word `0x0334` when bit 2 is a fault flag and bits
14–3 are quarter-degree counts.

**Answer.** `0x09C4 = 2500`, so valid temperature data means 25.00 C. The sensor
word contains 102 counts, corresponding numerically to 25.50 C, but its fault
bit is set. It must not be published as a valid measurement. Representation,
units, and validity are separate parts of interpretation.

**Further question.** Why is the synthetic -10.25 C value useful in a parser
test even if the example sensor produces nonnegative results?

**Answer.** It tests the protocol decoder's signed representation independently
of the physical source. Its bytes are `FF FB FF FF`, representing -1025.

## Chapter 2

**Problem.** Explain endpoint addresses `0x01` and `0x81`. An application block
has 800 bytes and an endpoint's maximum payload packet is 64 bytes. How many
data packets are required at minimum?

**Answer.** Both use endpoint number 1; `0x01` is OUT from the host and `0x81`
is IN toward it. `ceil(800/64) = 13`: twelve full payloads and one 32-byte
remainder. The application still defines one block, and one host read is not
guaranteed to return precisely that block.

**Further question.** Will connecting a full-speed RP2040 to a 40 Gbit/s port
increase its negotiated mode to 40 Gbit/s?

**Answer.** No. Endpoint hardware and firmware remain full speed. A connector
or host capability does not upgrade the device controller.

## Chapter 3

**Problem.** Decode `bmRequestType = 0xC0`. Explain how successful driver
binding can coexist with an unsupported application protocol.

**Answer.** Bit 7 selects device-to-host; request type bits select vendor;
recipient bits select device. OS binding establishes a transport route.
A separate private capability query can reject a device that lacks the
expected measurement operations, versions, or message lengths.

**Further question.** Why must a descriptor callback not return a local array?

**Answer.** The transfer can use the bytes after the callback returns. Automatic
local storage no longer has a valid lifetime. Persistent storage is required.

## Chapter 4

**Problem.** In a power supply, assign responsibility for millivolt units,
bulk transfer, and host device access. Is libusb Linux's equivalent kernel
driver to `winusb.sys`?

**Answer.** Firmware and product software define units and command semantics;
host APIs/drivers and controllers perform transport/access. libusb is a
user-space library above OS-specific backends, not a kernel function driver.
A comparable Linux user-space approach uses libusb plus Linux USB facilities.

**Further question.** Does switching a Windows application to libusb remove
its need for a suitable underlying device binding?

**Answer.** No. Its Windows backend can still use WinUSB. A portable user API
does not replace every OS component below it.

## Chapter 5

**Problem.** Name TinyUSB's main device-side layers. Explain why initializing
the stack does not make it an independent background thread.

**Answer.** Controller-specific DCD, device core, and class implementations
provide reusable machinery beneath application code/callbacks. In a cooperative
integration, the main loop must service the task path. Long delays between
calls defer work even though hardware interrupts can signal events.

**Further question.** Does a successful `tud_vendor_write` count mean the host
application parsed the payload?

**Answer.** It reports acceptance into the stack's transmit path. Transfer
submission, bus delivery, and host interpretation are later boundaries. A
partial count requires preserving the unsent suffix.

## Chapter 6

**Problem.** Decode raw SPI word `0x0550` into counts, degrees, and four
temperature-x100 bytes. Compare sixteen clocks at 1 MHz with a 250 ms
conversion allowance.

**Answer.** The extracted field is 170; temperature is 42.50 C; x100 is 4250,
or `0x109A`, encoded `9A 10 00 00`. Clocking takes approximately 16 microseconds.
The conversion allowance is 250,000 microseconds, so speeding up readout does
little to the overall cycle.

**Further question.** Name three plausible meanings of measurement timestamp.

**Answer.** Conversion start, readout, and host receipt are different examples.
The protocol must state which clock and event it means. Precision of storage
does not prove precision of physical sampling time.

## Chapter 7

**Problem.** Encode 42.25 C at uptime 2000 ms using illustrative token `00`
through `0F` in the example response format.

**Answer.** The complete response is:

```text
54 48 52 32 00 00 00 00 81 10 00 00 D0 07 00 00
00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
```

`0x1081` is 4225 hundredths; `0x07D0` is 2000 ms. For a fault, the numeric
temperature is invalid regardless of its storage bytes.

**Further question.** Why are correlation and fresh acquisition independent?

**Answer.** An old response can be rejected by token matching, but firmware
could still echo a new token on cached data. The device must also implement
the specified acquisition ordering.

## Chapter 8

**Problem.** Explain API versus ABI and trace cleanup if protocol negotiation
fails after successful Windows device opening.

**Answer.** API is the caller-visible operation/behavior contract; ABI is its
binary realization. After negotiation failure, release the WinUSB interface,
close the underlying device handle, and delete private state. Temporary path
storage and device-information sets have their own ownership/release rules.
Do not publish an initialized client handle on that failure path.

**Further question.** Why must device close precede DLL unload?

**Answer.** Cleanup and pending callbacks need loaded implementation code.
Function addresses and private destruction behavior cannot be used after the
module is released.

## Chapter 9

**Problem.** A 32-byte send accepts ten bytes, then zero, then twenty-two.
Describe each next offer. Why cannot a submitted asynchronous buffer be freed
as soon as cancellation is requested?

**Answer.** After the first call, offer offset 10/length 22; zero progress
retains that same suffix; the final count completes the frame. Pending I/O
can still complete into its buffer after cancellation is requested. Wait for
the API's completion/drain conditions before freeing storage.

**Further question.** Prove the rolling parser's count fits its next append.

**Answer.** Incomplete accumulation leaves count below 32; a rejected full
candidate becomes 31; a matching candidate resets to zero. Under normal
object use, the next append therefore writes positions 0 through 31 only.

## Chapter 10

**Problem 1.** Recalculate the eight-channel, 20 kscan/s stream using four
stored bytes/sample, 32 scans/block, and a 32-byte header.

**Answer.** Samples require 640,000 B/s. Payload is 1024 bytes/block; total is
1056. At 625 blocks/s, traffic is 660,000 B/s. Payload efficiency is
`1024/1056 = 96.97%`. Minimum packet count is seventeen at 64 bytes/packet.

**Problem 2.** Find free storage needed for a 40 ms service gap at 500,000 B/s.

**Answer.** `500,000 * 0.040 = 20,000` bytes, before existing queued work and
additional margin.

**Problem 3.** What duration does an empty 16 KiB buffer represent at that rate?

**Answer.** `16 * 1024 / 500,000 = 0.032768 s`, or 32.768 ms.

**Problem 4.** Production is 600,000 B/s; consumption is 500,000 B/s. Does
doubling 64 KiB capacity establish stability?

**Answer.** No. Growth remains 100,000 B/s. Empty-queue fill time changes from
0.65536 to 1.31072 seconds. Buffering delays overload without curing it.

**Problem 5.** Why add stage times for latency but take a minimum of independent
stage capacities for pipelined throughput?

**Answer.** One result traverses sequential delays. Overlapped independent
stages can work on different results concurrently, but the slowest limits
long-term service. Shared CPU or bus resources require a combined demand model.

## Chapter 11

**Problem 1.** Find stereo 48 kHz traffic using four bytes/sample.

**Answer.** `2 * 48,000 * 4 = 384,000 B/s`, or 384 bytes/ms.

**Problem 2.** Why does fitting an isochronous interval not prove an application
deadline?

**Answer.** It establishes one transport-capacity condition. Application
scheduling, clock behavior, successful delivery, and processing latency remain.

**Problem 3.** In the five-buffer example, what coverage remains after one
completion?

**Answer.** Four posted buffers give `4 * 4096 / 8,000,000 = 2.048 ms`.
Counting the completed buffer overstates future posted capacity.

**Problem 4.** Halve the example clock mismatch from 100 ppm to 50 ppm.

**Answer.** Drift becomes 2.4 stereo sample frames/s or 14.4 B/s. The same
initially empty 2880-byte buffer fills in 200 seconds under the model.

**Problem 5.** Define a recorder stop that preserves acquired data.

**Answer.** One valid contract reports a final acquired sequence, ceases
production, drains through that sequence, confirms storage or reports loss,
then retires I/O and memory. Other contracts are possible; “stopped” must name
a precise boundary.

## Chapter 12

**Problem.** Design separate tests for parser behavior, fresh conversion, and
resource cleanup.

**Answer.** Feed controlled fragmented/stale-token bytes to the actual parser;
observe conversion restart/readout and changed temperature after idle time;
then stop a real session and verify a new client can open it. Each establishes
its own boundary. None alone proves calibration, every removal race, or a
hard real-time deadline.

## Five practical experiments

1. **Representation.** Construct the complete Chapter 7 response on paper,
   then compare its bytes with the arithmetic checker.
2. **Parser.** Run the existing parser suite. In an experimental branch, vary
   every split position in one frame and add a matching reserved-byte error.
3. **Acquisition.** Run three live reads with long pauses. Change temperature
   during a pause and record timing/status rather than assuming the example
   values are measurements.
4. **Lifetime.** Request Ctrl+C during a read and during a pause, then reopen.
   Record the behavior; do not close from another thread in violation of the
   current handle contract.
5. **Capacity.** Collect a longer timing run under a defined workload. Report
   distribution and maximum observed, then distinguish those observations from
   a proven bound. Design a hypothetical stream's queue/loss policy on paper.

Keep changes isolated and retain the normal firmware. The silent historical
probe is not a timeout test for the current DLL: it fails capability checking
before the read. A current-compatible experimental image that withholds
replies exercises a different path.

<!-- NAVIGATION -->

---

[Previous: 12. Case study, experiments, and further driver work](12-thermohost-case-study.md) · [Contents](README.md) · [Next: Appendix B. Software provenance and source atlas](appendix-b-source-atlas.md)
