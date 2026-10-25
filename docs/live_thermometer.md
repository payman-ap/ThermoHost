# MAX6675 acquisition over WinUSB

## Firmware target

`thermometer_usb` combines the previously tested MAX6675 SPI acquisition and
10-byte encoding with the tested TinyUSB vendor interface and descriptors.
The original `firmware` CDC target and `usb_probe` constant-frame target remain
available as separate diagnostics.

```powershell
cmake --build build --target thermometer_usb
```

Output: `firmware/build/thermometer_usb.uf2`.

## Timing and data

- CS is initialized high; SPI0 uses GP0 RX, GP2 clock, GP1 software CS.
- The first read waits at least 250 ms after initialization.
- Each 16-bit SPI read ends by raising CS. The next conversion deadline is
  set 250 ms later, independently of host activity.
- The main loop keeps calling `tud_task_ext(0, false)` and sleeps for 1 ms
  between iterations rather than blocking USB for the conversion interval.
- Valid quarter-degree counts become integer hundredths with `counts * 25`.
- Open thermocouples set flag bit 0 and encode a zero temperature field.
- A sample is delivered only when mounted and space for all 10 bytes is
  available. Backpressure skips delivery rather than blocking acquisition.
- Device timestamps describe sample retrieval time since boot. They are
  encoded, but the current public DLL API does not expose them.

## Compatibility

The vendor interface, endpoint addresses, VID/PID, interface GUID, and wire
format match the probe firmware. No DLL or console edits are needed for this
step. The separate fixed-value helper in the DLL still returns 25 C; only the
`USB sample` lines are actual USB measurements.

## Verification

- Agent built and linked this target successfully with the installed Pico SDK
  2.3.1 and ARM toolchain.
- User flashed this target and supplied a complete 20-sample console run.
- All samples returned status 0, with initial values around 23.00–23.75 C
  followed by readings around 28.50–28.75 C. The changing data demonstrates
  actual acquisition rather than the previous fixed 42.25 C test frame.
- Initial reads returned immediately, consistent with queued data; subsequent
  samples took approximately 235–265 ms each.
- Null-argument checks and handle cleanup also completed successfully.
- This verifies delivery of sensor data, not thermometer calibration.

- User-verified integrated fault test: after queued valid samples, sample 8
  returned SensorFault (4) with the caller's output still -999. Argument checks
  and cleanup completed normally.
- The user restored the thermocouple connection and confirmed normal readings
  returned.

## Freshness limitation

Reads return the next queued complete sample, not necessarily the latest one.
The fault test delivered seven older valid readings before the fault frame,
consistent with buffering on the device/host path. This can delay visibility
of changing temperature or sensor faults when a client starts or falls behind.

Do not interpret a successful read as a guarantee of current sensor state.
Device timestamps are already on the wire; exposing them and defining an
explicit freshness policy are subsequent steps. No stream resynchronization
after data loss or automatic hot-plug recovery is implemented yet.

## Milestone

Tag `live-thermometer-winusb` preserves this verified sensor-to-console chain
before changes to buffering and freshness semantics.
