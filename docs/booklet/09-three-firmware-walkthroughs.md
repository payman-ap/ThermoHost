# 9. Three firmware applications as controlled experiments

## Why keep the earlier programs?

A complete system can fail in many places. A small diagnostic program removes
some of them. This repository's three targets form a useful progression:
establish sensor acquisition, establish USB transport with known data, then
combine acquisition with a request-driven protocol. They are independent
experiments, not a boot menu.

| Program | Real sensor? | Data production | Intended reader |
| --- | --- | --- | --- |
| `firmware` | Yes | Periodic text and packet hex dump. | Serial terminal. |
| `usb_probe` | No | Periodic synthetic binary frames. | Matching historical host tools. |
| `thermometer_usb` | Yes | One fresh response per accepted request, subject to superseding commands. | Current DLL API 3. |

The last qualification matters: a later command can replace an unfinished
conversion in the current firmware. Normal serialized DLL use sends one
request and waits, so this is not a multi-request queue.

## The original sensor application

Read [firmware.c](../../firmware/firmware.c). `stdio_init_all()` initializes
enabled SDK standard-I/O channels; CMake enables USB stdio and disables UART.
The program configures chip select and SPI, schedules the first read 250 ms
ahead, then repeatedly waits and reads two bytes.

It stores raw data, validity, and a `double` for human-readable output. On a
valid reading it prints the raw hexadecimal word and temperature. On an open
thermocouple it prints a fault message. These are diagnostics, not the current
DLL API.

It also constructs a ten-byte frame: type, flags, four temperature bytes, and
four timestamp bytes. Then it prints each byte using `%02X`. A terminal sees
readable characters such as `01 00 F6 09 ...`; the custom WinUSB decoder does
not receive that array directly.

The next read is scheduled relative to the time after the SPI transaction.
The loop is therefore approximately periodic rather than a precision
metronome. Formatting and USB stdio can add overhead. This target is a
measurement/debugging exercise, not a deterministic acquisition engine.

## The USB probe's known frame

Read [usb_probe.c](../../firmware/usb_probe.c). It does not initialize SPI.
It makes GP1 high to keep the connected MAX6675 deselected. Then it initializes
TinyUSB in full-speed device mode and services USB in a loop.

```c
uint8_t packet[10] = {
    0x01, 0x00,
    0x81, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};
```

The temperature field is little-endian `0x00001081 = 4225`, meaning 42.25 C.
The final four bytes are overwritten with current uptime for each transmission.
An increasing timestamp shows that packets were produced over time; it does
not turn the fixed temperature into a sensor reading.

Every 250 ms, the program checks `SEND_TEST_PACKETS`, interface readiness, and
space for a whole ten-byte packet. It queues the packet and flushes if the
write accepted all bytes. The whole-frame space check is suitable for this
small diagnostic. The live target uses an explicit sent-byte count instead.

The unsigned difference `now_ms - last_send_ms` handles a single uptime wrap
for these short intervals. After delay or backpressure it does not produce an
unbounded catch-up burst: it updates the last-send time to the current time.
Some opportunities can be skipped if the interface cannot accept a frame.

## The silent-probe exercise is historical

Setting `SEND_TEST_PACKETS` to zero leaves USB active without producing test
data. With the matching older host version, this demonstrates a read timeout
without disconnecting the cable. The current DLL deliberately rejects this
firmware at capability negotiation, so it cannot reproduce that older read
experiment unchanged.

This is a general lesson in versioned testing. A valid test procedure for one
checkpoint may exercise a different path after compatibility checks are added.
Always ask whether failure occurs at open or during a read.

## The live application combines two proven boundaries

Read [thermometer_usb.c](../../firmware/thermometer_usb.c). It retains the sensor
SPI operations and vendor descriptors, but changes acquisition policy. A valid
request starts a new conversion. A deadline allows USB servicing during the
wait. The response echoes a token and reports a fault or temperature.

It does not periodically call the sensor in the background. It does not return
the most recent cached reading immediately. It does not expose both the CDC
terminal and vendor interface. These design choices make the beginner's
freshness claim easier to reason about.

An idle MAX6675 can still be doing its internal work with CS high. “No idle
measurement stream” means that the application does not read and transmit
periodic samples to the host. It is not a claim that every physical component
stops operating between commands.

## Time-source hooks and library callbacks

Both vendor targets implement `tusb_time_millis_api()` using Pico uptime.
Library callbacks are functions your application defines for the stack to call.
They differ from SDK functions the application invokes. Descriptor callbacks
follow the same inversion: TinyUSB asks the application for descriptor storage.

The installed TinyUSB version matters. The source supplied with Pico SDK 2.3.1
has a void-returning `tud_task_ext` implementation. Newer online documentation
may describe additional behavior. The booklet explains the checked-in calls
against the installed stack, not an assumed latest API signature.

## Check your understanding

1. Why is fixed 42.25 C useful even though it measures nothing?
2. Why does a serial hex dump not test WinUSB bulk transfer?
3. What would the current DLL do with a silent probe?
4. Why should a periodic probe avoid assuming every 250 ms opportunity was sent?

Answers: [Chapter 9](24-answers.md#chapter-9).

<!-- NAVIGATION -->

---

[Previous: 8. Building, flashing, linking, and loading](08-build-flash-and-load.md) · [Contents](README.md) · [Next: 10. The live firmware as a state machine](10-live-firmware-state-machine.md)
