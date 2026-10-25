# Session 2: MAX6675 over SPI

Date: 2026-10-25

## Wiring and configuration

| MAX6675 | RP2040-Zero |
| --- | --- |
| VCC | 3V3 |
| GND | GND |
| SO | GP0, SPI0 RX |
| CS | GP1, software-controlled GPIO |
| SCK | GP2, SPI0 SCK |

- `hardware_spi` linked alongside `pico_stdlib`.
- SPI0 configured for 8-bit transfers, mode 0, MSB first.
- Requested clock: 1 MHz; SDK reported configured clock: 992063 Hz.
- Read two bytes with CS low, then return CS high for conversion.
- Allow at least 250 ms with CS high before reading; specified maximum
  conversion time is 220 ms. Use `sleep_until(next_read)` and set the next
  deadline with `make_timeout_time_ms(250)` immediately after raising CS.
  Decoding and printing happen during this conversion interval. If processing
  takes longer than 250 ms, the next read happens later without catch-up reads.
- Explicitly assemble high byte first into a 16-bit unsigned value.
- Temperature counts: `(raw >> 3) & 0x0FFF`, each count is 0.25 degrees C.
- Bit 2 indicates an open thermocouple; reject temperature when it is set.

## Results

- Initial room-temperature readings were about 24.25–26.00 degrees C.
- User heated the probe with a lighter; readings reached about 52 degrees C
  and subsequently decreased. This demonstrates response, not calibration.
- Removing a thermocouple lead produced repeated open-probe reports,
  including raw value `0x7FFC`.
- Some samples around the disconnect/reconnect test showed large decoded
  temperatures with the fault bit clear. Their cause was not established;
  do not treat these as verified physical temperatures.
- After being asked to restore the probe and check normal readings, the
  user reported that all was fine.

## Learning points

- SPI completes transfers even without a responding sensor: there is no ACK.
- Two bytes are transferred while CS remains low for the entire reading.
- The sensor is read-only; no MOSI wire is required.
- Check the fault bit before using the temperature field.
- USB CDC is currently used for diagnostics; custom WinUSB comes later.

## Completed Session 2 practice

- `TemperatureReading` stores raw data, decoded temperature, and validity
  using automatic storage without heap allocation.
- A fault clears the stored temperature to a placeholder; consumers must
  check `valid` before using it.
- Deadline-based polling lets printing overlap the sensor's conversion time.
- During review, corrected `sleep_ms(next_read)` to `sleep_until(next_read)`:
  the former takes a duration, while the latter takes an absolute deadline.
- The user confirmed successful operation after both the struct and timing
  changes, with readings continuing approximately four times per second.

Session 2 is complete. Session 3 (Qt over CDC) is optional; the next required
transport step is Session 4 (custom TinyUSB device and WinUSB).

## Reference

MAX6675 datasheet:
https://www.analog.com/media/en/technical-documentation/data-sheets/MAX6675.pdf
