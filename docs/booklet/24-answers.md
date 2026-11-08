# 24. Worked answers and reasoning checks

Try the questions before reading this guide. The reasoning is more useful than
memorizing a number. Where an answer proposes future behavior, it does not
claim that behavior is implemented.

## Chapter 1

1. `Thermo_ReadTemperature` runs in the Windows process on the PC.
   `max6675_read_raw` runs on the RP2040. The wire contains copied bytes between
   their separate address spaces.
2. The board can read the sensor fault bit, encode a valid current response,
   and send it successfully. Communication success and measurement validity
   are different conditions.
3. The API version describes exported-function semantics; wire version
   describes transported bytes. Their numbers have independent histories.
4. Starting at the caller, a writable `double` is eventually filled by the
   parser from signed fixed-point bytes. Those bytes were encoded from raw SPI
   counts after a requested conversion. The actual chronological flow starts
   with the request, not with the result pointer being written.

## Chapter 2

1. `C4 09 00 00` is `0x09C4 = 2500`, so the value is 25.00 C if flags and
   identity are valid.
2. `0x0554 >> 3` gives 170 counts, but bit 2 is set. The candidate temperature
   bits correspond to 42.50 C; the measurement must be reported as a fault.
3. The magic occupies exactly four bytes. Its C string terminator is storage
   syntax, not part of the protocol. Sending it shifts all following fields.
4. The response includes metadata and one temperature field. A byte is storage,
   not a measurement-count unit.

## Chapter 3

1. The SPI clock only shifts a completed word. A new conversion needs the
   converter's much longer time interval.
2. Two decimal places describe formatting. Sensor steps are 0.25 C; accuracy
   requires calibration/specification and experimental context.
3. A pointer names an address in one machine's process or firmware memory.
   The other machine has unrelated storage and address translation.
4. Conversion start, converter completion/readout, host receipt, and display
   time are all possible timestamps. A protocol must define which it means.

## Chapter 4

1. CS is deliberately toggled by application logic for selection and restart.
   The SPI peripheral handles data and clock on GP0/GP2.
2. It is the repeated dummy transmit byte used to generate clocks, not a
   timeout, device address, or temperature command.
3. `05 50` becomes raw `0x0550`, counts 170, temperature 42.5, x100 value 4250,
   and little-endian field `9A 10 00 00`.
4. Restarting establishes that the requested conversion began after the
   command, instead of merely reading a previously available result.

## Chapter 5

1. Both addresses use endpoint number 1. `0x81` has direction bit 7 set and
   carries data toward the host. `0x01` carries data away from the host.
2. A FIFO capacity or packet maximum is a transport/storage constraint. Your
   response protocol contains one reading in thirty-two bytes.
3. Link retries preserve byte delivery. They do not know which request a
   payload belongs to. A perfect old response is still old.
4. No. Firmware can change descriptors and behavior on the same native USB
   hardware, as this project demonstrates.

Additional chapter 5 questions: budget the actual full-speed link at 12 Mbit/s,
not the specification family's 480 Mbit/s high-speed mode. A charging-power
label leaves data mode, cable capabilities, host/device support, and actual
payload behavior unspecified. Neither connector shape nor charging wattage
proves a particular data throughput.

## Chapter 6

1. Class information is supplied by interfaces when the whole-device class is
   zero in this arrangement.
2. The descriptor source is shared. The live build's capability control
   response distinguishes it from the probe.
3. The first descriptor has zero-based index 0; its selected configuration
   value is 1.
4. The stack uses the returned bytes during transfer processing after the
   callback returns. A local automatic array would no longer be valid storage.

## Chapter 7

1. ThermoDevice knows the thermometer contract. Winusb.dll exposes Windows
   USB API functions. Winusb.sys serves as the kernel-mode function driver.
2. It points to your C++ private object, not a Windows handle. `Thermo_Close`
   releases that object's contained resources in the correct order.
3. Binding establishes an accessible transport. The capability query checks
   whether the expected application protocol is present.
4. Current `WinUsb_ReadPipe` and `WinUsb_WritePipe` calls pass a null
   `OVERLAPPED` pointer. The opened handle still has the required overlapped
   capability flag.

## Chapter 8

1. Each target compiles its source with its own definitions. The live target
   includes the protected capability block; the probe does not.
2. Include paths locate source headers while compiling. DLL lookup locates
   executable modules while the Windows process runs.
3. Copy `thermometer_usb.uf2` to the BOOT-mode drive. `ThermoConsole.exe` and
   `ThermoDevice.dll` run on Windows.
4. A built file can remain on disk without being copied to the board; the
   board continues running the previously installed image.

## Chapter 9

1. Known bytes remove the physical sensor and SPI as variables. They let you
   investigate transport and decoding with an exact expected result.
2. CDC text output uses different descriptors, driver behavior, and payload
   encoding from vendor bulk USB.
3. The capability query fails the exact expected reply check, producing
   UnsupportedDevice during open rather than a read timeout.
4. The send condition also requires mount state and sufficient FIFO capacity.
   A scheduling opportunity is not proof of accepted or received data.

## Chapter 10

1. One counter tracks incoming command assembly; the other tracks bytes of an
   outgoing response already accepted by TinyUSB.
2. The later command's token replaces the previous token and its restart
   resets the conversion deadline. Pending not-yet-queued response work is
   superseded. Previously queued bytes may still exist in transport.
3. A long sleep in this loop would defer its regular USB task servicing.
   Checking a deadline allows useful work during the wait.
4. Clearing after `sent == sizeof(packet)` proves the response was accepted
   into the stack's transmit path. It does not prove Windows parsed it.

## Chapter 11

1. 42.25 C gives x100 `4225 = 0x1081`; 2000 ms is `0x07D0`.

   ```text
   54 48 52 32 00 00 00 00 81 10 00 00 D0 07 00 00
   00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
   ```

2. A fault frame deliberately carries zero temperature storage. Flags decide
   whether the numeric field is meaningful.
3. Firmware restarts conversion after receiving a command; the host accepts
   only a reply echoing the current request's token.
4. Negotiate a new compatible contract and update encoder, decoder, capability,
   specification, and tests. An older decoder intentionally rejects that byte.

## Chapter 12

1. Bit 7 is IN, type bits are vendor, recipient bits select device.
2. `0x20` retrieves the OS descriptor set with index 7. `0x22` retrieves the
   five-byte application capability with index/value zero.
3. Ten-byte set header plus twenty-byte compatible ID plus 132-byte registry
   property equals 162 (`0xA2`).
4. Driver selection and interface discovery do not imply the fresh-conversion
   command is implemented.

## Chapter 13

1. Status separately expresses transport, argument, and sensor outcomes;
   temperature storage remains unchanged unless valid data is published.
2. Opaqueness hides layout and resources. It does not prevent dangling,
   fabricated, or multiply closed pointers.
3. The loaded DLL contains the cleanup implementation, and its function
   addresses become unusable after unload.
4. Another thread inside the same process can still race on the same mutable
   object. Exclusive file sharing is not a mutex around function calls.

## Chapter 14

1. The code returns failure for index zero; it does not search index one.
2. The first call deliberately asks for the required variable buffer size
   without supplying storage.
3. Share mode zero governs other opens. A mutex governs concurrent use of
   already shared in-process state.
4. Open the path, initialize WinUSB, query setting zero, verify vendor class,
   find both bulk directions, verify capability, and set read policies.

## Chapter 15

1. Each newly granted second could extend the overall wait dramatically,
   especially while scanning old data. Remaining-budget calculations prevent
   intentional budget renewal on every read.
2. The parser skips unknown prefixes and tokens. Only a matching candidate
   with invalid checked fields produces InvalidPacket.
3. No. Open and its control transfer are outside this function's budget.
4. Random bytes can contain zero. C string comparisons would stop before
   comparing all sixteen bytes.

## Chapter 16

1. Incomplete accumulation returns with count below 32. A full rejected window
   becomes 31; a matched window resets to zero. Therefore the next write stays
   within the array under normal object use.
2. A valid response might start one byte into the rejected window. Keeping
   the suffix lets the next append reveal it.
3. The protocol decoder is broader than the physical sensor's generated
   positive range. Synthetic byte tests establish decoder behavior only.
4. The timestamp at offsets 12–15 is not interpreted or exposed by `Feed`.

## Chapter 17

1. Zero is an internal sentinel. The CLI deliberately requires a positive
   finite limit when the option is specified.
2. Main may be in an active read. Closing from the handler would violate the
   DLL contract and race its state. The flag lets main own cleanup.
3. About `3 * 0.25 + 2 * 20 = 40.75` seconds plus overhead.
4. The loop permits SensorFault and counts attempts, so it can finish its
   requested count without a transport failure or valid physical readings.

## Chapter 18

1. It supplies synthetic bytes; it cannot validate wiring, conversion start,
   calibration, bus behavior, or the flashed image.
2. Keep a current compatible device connected but silent for timeout. Remove
   it for disconnect. Record the failing stage and status separately.
3. A token is meaningful through correspondence. Without the request you
   cannot establish whether the response belongs to the active read.
4. Fresh-version fault/recovery and Ctrl+C board checks are unconfirmed in
   the cited local record.

## Chapter 19

1. A matched response can still be delayed after acquisition; no clock/age
   contract is exposed by the temperature-only API.
2. Idealized unbounded queue growth is `(4 - 1) * 10 = 30` samples. Real
   finite buffers impose additional block/drop behavior.
3. Overlapping work improves use of time, but neither sensor, OS, nor bulk
   scheduling gains a proven hard deadline from that API style.
4. One valid example: 4 Hz acquisition, display only results no more than
   1 second old, tolerate explicit drops, show a stale indicator on deadline
   miss. Verifying it would need additional timing semantics and evidence.

## Chapter 20

1. Normally it sends requests through the supplied USB stack; it does not
   replace the host-controller driver just to read this thermometer.
2. Buffers and objects must survive pending I/O; completion, cancellation,
   close, and removal paths must release each resource exactly once.
3. Retrying can repeat a side effect. The protocol must define operation
   identity and idempotency rather than assuming all commands behave like reads.
4. A baseline separates new driver-model mistakes from board/protocol faults
   and preserves a way to compare observed behavior.

## Extended WinUSB design

For the hypothetical power supply, product firmware/library define units and
calibration meaning; the host library defines command identity and retry
policy; the operating system and firmware descriptors establish binding; the
device and host together define safe shutdown behavior. Moving to Linux changes
host access/discovery code, not the physical meaning of a millivolt or whether
repeating an actuator command has a side effect. See
[Chapter 7](07-what-winusb-is.md).

## Chapter 25

1. A portable private device object still corresponds conceptually to
   `ThermoHandle`. A libusb device handle plus claimed-interface state supports
   host transfers, but is not a type-compatible WinUSB handle. A dynamically
   loaded Linux shared object could have a module reference, but a port need
   not dynamically load its library at all; `HMODULE` has no mandatory one-to-one
   counterpart in the proposed transport algorithm.
2. USB setup fields and application bytes are independent of host API names.
   Sending the same request tuple reaches the same device callback.
3. Different backends can have different packet/overflow/retention behavior.
   Receive suitable chunks, then feed bytes to the parser; do not assume
   WinUSB pipe policy is a portable API contract.
4. Permission failure means the process lacks access; interface ownership
   means another driver/client has a claim; protocol mismatch means access
   exists but the device does not provide the expected application behavior.
5. No. A libusb Windows backend still needs a compatible underlying driver
   arrangement and may use WinUSB itself.

## Chapter 26

1. Four-byte storage gives 640,000 B/s of samples. Each 32-scan payload is
   `8 * 32 * 4 = 1024` bytes, plus 32 header bytes = 1056. At 625 blocks/s,
   application traffic is `1056 * 625 = 660,000 B/s`. Efficiency is
   `1024 / 1056 = 96.97 percent`. It requires at least seventeen 64-byte data
   packets per complete block: sixteen full packets and a 32-byte remainder.
2. `500,000 * 0.040 = 20,000` free bytes, before other queued data and margin.
3. `16 * 1024 / 500,000 = 0.032768 s`, or 32.768 ms if initially empty.
4. No. The difference remains 100,000 B/s. Fill time changes from 0.65536 to
   1.31072 seconds for initially empty buffers; the average mismatch remains.
5. Adding times models a result traversing sequential stages. Taking a minimum
   of independent capacities models sustained throughput after overlapping
   stages are active. Shared-resource demands can invalidate the independence
   assumption and must be budgeted separately.

## Chapter 27

1. `2 * 48,000 * 4 = 384,000 B/s`, or 384 bytes/ms.
2. It establishes one transport-capacity condition, not application scheduling,
   complete clock behavior, successful transfers, or hard deadlines.
3. Four buffers remain: `4 * 4096 / 8,000,000 = 0.002048 s`. The just-completed
   buffer no longer represents posted future receive coverage. Counting all
   five would overstate immediately available queued capacity.
4. Drift is half as large, 2.4 sample frames/s or 14.4 B/s. The same empty
   2880-byte buffer fills in 200 seconds under the simplified model. An
   initially half-full buffer has half that one-direction margin.
5. One defensible contract: stop acknowledgement specifies the final acquired
   sequence, production ceases, transport drains every sequence through that
   boundary, host storage confirms them or reports an explicit failure, and
   only then may outstanding requests and buffers be retired. Other contracts
   are possible, but “stop returned” must have a precise meaning.

## Putting the course together

Explain one successful request without using the phrase “USB magic.” Then
explain an old-token response, a current sensor fault, a missing capability,
and an unplug during reading. For each, name the responsible state and the
output-storage behavior. If you can do that and justify the timing limits,
you understand the core system well enough to begin a bounded next experiment.

<!-- NAVIGATION -->

---

[Previous: 23. Source atlas and primary reading](23-source-atlas-and-reading.md) · [Contents](README.md) · [Next: 25. Linux, libusb, and a portable thermometer library](25-linux-and-portability.md)
