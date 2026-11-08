# 5. USB without the mystery

## Host and device are different roles

The PC is the USB host; the RP2040 is the USB device. The host schedules bus
communication. Even when firmware appears to “send” a reading, it queues data
for a host IN transaction. Calling `tud_vendor_write` does not allow the board
to seize the bus and transmit whenever it likes. This host-driven model is a
fundamental difference from a pair of UART transmitters.

TinyUSB's [USB concepts guide](https://docs.tinyusb.org/en/stable/reference/usb_concepts.html)
introduces these roles. Keep the direction convention fixed: **IN means toward
the host; OUT means away from the host**. Thus the board receives your command
on OUT and supplies its result on IN.

## Endpoint, pipe, interface, configuration

An endpoint is a device-side source or sink for transfers. A pipe is the
host-side software relationship used to access it. An interface groups related
functionality. A configuration groups interfaces into an operating arrangement.
The current thermometer has one configuration, one vendor interface, and two
bulk data endpoints, plus the default control endpoint zero.

The endpoint address includes a direction bit. `0x01` and `0x81` share endpoint
number 1 but have opposite directions. They are not the same unidirectional
endpoint. Endpoint zero is special: control transfers use it for bidirectional
control exchanges. Microsoft explains the distinction between
[endpoints and pipes](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-endpoints-and-their-pipes).

The names help you interpret actual code. `BulkInPipe` is a stored endpoint
address used with a WinUSB interface handle. It is not a pointer to a buffer on
the RP2040. `tud_vendor_mounted()` checks whether TinyUSB's vendor interface is
ready, not whether a PC application currently has an open device handle.

## Four transfer types

| Type | General purpose | Role here |
| --- | --- | --- |
| Control | Configuration and device-specific control operations. | Descriptors and fresh-protocol capability query. |
| Bulk | Reliable data delivery using available bus bandwidth. | Command and response bytes. |
| Interrupt | Periodically serviced transfers for bounded-size data. | Not used. The name does not mean an application CPU interrupt. |
| Isochronous | Scheduled periodic data with different error/retry tradeoffs. | Not used; relevant to later audio/acquisition study. |

Bulk is suitable for this low-rate thermometer because missing or corrupt
application data should not silently become a temperature. USB handles link
error detection and retry machinery below your application bytes. However,
bulk bandwidth is not reserved, so successful delivery has no guaranteed
application latency. See Microsoft's
[bulk-transfer overview](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-bulk-and-interrupt-transfer).

Your token check solves a different problem from USB link error detection. An
old response can be delivered perfectly and still be the wrong response for
the current request. Reliability of bytes does not establish relevance of data.

## Bits, packets, transfers, and messages

There are several sizes in play. The device uses full-speed USB. The bulk
endpoint maximum packet size is 64 bytes. The application request is 20 bytes;
its response is 32 bytes. TinyUSB also allocates 64-byte software buffers.
These equal-looking numbers describe different objects.

A USB data packet is a bus-level unit. A host transfer can involve packets and
completion rules. Your application frame is a protocol message. A parser must
work when data reaches it in pieces. One successful `ReadPipe` call is not a
general proof that exactly one complete application frame arrived.

The current DLL deliberately asks for one byte per call. With its chosen pipe
policies, WinUSB retains excess bytes for subsequent reads. A single 32-byte
device response therefore becomes many parser calls; it does not require the
device to manufacture thirty-two separate one-byte USB messages.

This distinction becomes critical when moving to larger frames. Suppose a
future message is 200 bytes. It cannot fit in one 64-byte full-speed bulk data
packet, but that does not make it an invalid application message. The protocol
must define how the receiver recognizes its complete 200-byte message.

## A conceptual bus exchange

For a bulk OUT transaction, the host identifies the target and supplies data;
the device acknowledges acceptance according to the bus protocol. For bulk IN,
the host requests data; a device with nothing ready can respond in a way that
causes the host to try later. A temporarily empty endpoint is not necessarily
a broken device. Software timeout policy decides how long the application
continues waiting.

At this layer, `THQ2` and `THR2` have no universal USB meaning. They are just
payload bytes. The standard bus protocol does not know they refer to a
temperature conversion, just as Ethernet does not inherently know the meaning
of an application JSON field.

## Speeds do not predict sensor throughput

Before discussing speed, distinguish the application bytes from the electrical
signal. USB's D+ and D- form a differential data pair, not separate transmit
and receive wires like a UART. Full-speed signaling uses NRZI encoding:
a data zero causes a transition and a one does not. Bit stuffing introduces
extra transitions for synchronization; hardware removes the inserted bits.
You do not add them to `packet[]`.

Bus packets also carry identifiers and error-checking fields. Token packets
identify a device and endpoint; data packets carry payload and a CRC. ACK
indicates acceptance, NAK can indicate temporary unreadiness, and STALL reports
a different endpoint condition. These bus-level terms are independent of your
sixteen-byte application token. Infineon's
[USB 101 application note](https://www.infineon.com/dgdl/Infineon-AN57294_USB_101_An_Introduction_to_Universal_Serial_Bus_2.0-ApplicationNotes-v09_00-EN.pdf?fileId=8ac78c8c7cdc391c017d072d8e8e5256)
illustrates this physical/protocol machinery.

Your encoder begins with the application magic, not the bus synchronization
pattern. Your parser receives application data after lower layers have handled
that machinery. Adding a bus CRC to the existing thirty-two-byte response
would therefore change the application protocol rather than “complete” its
USB packet. A new application checksum, if needed for another design, would
require a separately specified field and validation rule.

### Sensor rate versus link rate

Full-speed USB's signaling rate is 12 Mbit/s. This is neither 12 million
temperature samples per second nor 1.5 MB/s of guaranteed application payload.
Protocol overhead, scheduling, software, and the sensor matter. More
importantly, this design waits a quarter second for each conversion.

At about four reads per second, payload demand is approximately
`4 * (20 + 32) = 208` bytes per second, excluding USB overhead. Optimizing bus
bandwidth is not the first priority. Correct freshness, useful diagnostics,
and predictable ownership are much more valuable in this version.

## USB class does not mean C++ class

A USB class defines a family of device behavior understood by a driver. CDC
provides the serial-terminal personality in the original firmware. The current
interface uses vendor class `0xFF`, meaning that its application protocol is
device-specific. Vendor class does not inherently mean “WinUSB”; descriptors
and Windows binding behavior establish that relationship here.

The same microcontroller can expose different personalities depending on its
firmware. That explains why a COM port disappears after flashing the live
vendor firmware without implying that USB has failed.

## USB generations: specification, mode, connector, and result

Four descriptions are often collapsed into the word “USB”: a specification
family, a supported signaling mode, a connector, and an observed application
rate. Keep four columns in your engineering notes. A Type-C connector describes
a physical connection; a specification version describes supported protocol
features; a negotiated mode describes the actual link; payload throughput
describes useful bytes delivered over time.

For orientation, this table lists well-known nominal signaling-rate classes.
It is not a table of guaranteed file-copy speed or a complete compatibility
matrix. Decimal units are used: 1 Mbit/s is 1,000,000 bit/s.

| Family / mode | Nominal rate | Engineering interpretation |
| --- | --- | --- |
| USB low speed | 1.5 Mbit/s | Small low-rate devices; not the thermometer's bulk mode. |
| USB full speed | 12 Mbit/s | The RP2040's native runtime connection here. |
| USB 2.0 high speed | 480 Mbit/s | Requires high-speed-capable device/controller hardware. |
| USB 3.2 Gen 1 | 5 Gbit/s | Earlier names include USB 3.0 and USB 3.1 Gen 1. |
| USB 3.2 Gen 2 | 10 Gbit/s | A higher-rate 3.x mode. |
| USB 3.2 Gen 2x2 | 20 Gbit/s | Two-lane 3.x operation; exact device/host/cable support matters. |
| Original USB4 capabilities | 20 or 40 Gbit/s | Multi-protocol architecture; not merely a larger USB 2.0 bulk packet. |
| USB4 version 2.0 capability | Up to 80 Gbit/s symmetric | Optional asymmetric operation can allocate 120 Gbit/s one way and 40 the other. |

For the USB 2.0 speed distinctions and signaling model, see the vendor
[USB 101 introduction](https://www.infineon.com/dgdl/Infineon-AN57294_USB_101_An_Introduction_to_Universal_Serial_Bus_2.0-ApplicationNotes-v09_00-EN.pdf?fileId=8ac78c8c7cdc391c017d072d8e8e5256).
The 3.x mapping is documented by
[USB-IF's 3.2 terminology guide](https://www.usb.org/sites/default/files/usb_3_2_language_product_and_packaging_guidelines_final.pdf).
For USB4, use the [original system overview](https://www.usb.org/sites/default/files/D1T1-3%20-%20USB4%20System%20Overview.pdf)
and [version 2.0 announcement](https://www.usb.org/sites/default/files/2022-10/USB-IF%20USB%2080Gbps%20Announcement_FINAL_v2.pdf).
These historical specification names remain useful when interpreting datasheets;
consumer-facing naming can instead state a speed explicitly.

### Example: a faster host port does not redesign the RP2040

Plug this project into a hypothetical port capable of 40 Gbit/s. Its RP2040
USB controller and firmware still use full-speed USB. A hub or host cannot turn
the device into a high-speed peripheral through a new endpoint-size constant.
The MAX6675 still needs its conversion interval as well.

Compute the ratio of nominal rates: `40,000 Mbit/s / 12 Mbit/s` is about 3333.
It would be incorrect to predict a 3333-fold increase in thermometer sample
rate. Neither the device link mode nor the converter changed. This example
shows why comparing a port label directly to sensor sample rate is a category
error.

### Example: USB 2.0 does not mean every device runs at 480 Mbit/s

The project advertises a USB revision in its device descriptor and selects
full speed in initialization. The specification revision field is not a
speed command. High-speed operation requires different hardware support and
appropriate stack configuration. Device, host, topology, and cable must have
the required capabilities; a label on only one component is insufficient.

The direction of backward compatibility is also not a promise that every
optional high-rate mode exists in every newer host. Check the specific mode,
not merely whether the marketing version number is larger. For this project,
the simple decisive fact is the endpoint device's full-speed-only controller.

### What changes for a programmer at higher rates?

Application framing still matters, but endpoint packet limits, scheduling
units, descriptors, host APIs, and controller buffering can differ. Full-speed
and high-speed bulk maximum packet sizes differ; at high rates software
allocation, memory copies, and request queue depth can become important.
Do not copy a full-speed scheduling formula into a SuperSpeed calculation.

USB4 also transports multiple protocol types through a routed architecture.
An advertised fabric capacity is not a reservation of that entire capacity
for one instrument's application data. The engineering habit is unchanged:
identify the path and its bottleneck, then measure payload delivered at the
boundary your application actually uses.

For hand calculations that distinguish line rate, packet payload ceiling,
application overhead, and sustainable throughput, continue to
[Chapter 26](26-capacity-and-bottleneck-calculations.md).

## Questions for the expanded USB model

1. Explain both `0x81` and `0x01` from the host's perspective.
2. Why does a full 64-byte FIFO not mean a 64-byte sensor measurement?
3. Why can USB retry machinery coexist with stale application responses?
4. Does changing from CDC to vendor USB necessarily require different hardware?

5. A device reports USB specification revision 2.10 and uses full-speed
   endpoints. Should you budget 12 or 480 Mbit/s, and why?
6. A Type-C cable advertises charging power. What information is still missing
   before you can predict application data throughput?

Answers: [Chapter 5](24-answers.md#chapter-5).

<!-- NAVIGATION -->

---

[Previous: 4. SPI: from pins to a sensor word](04-spi-from-pins-to-word.md) · [Contents](README.md) · [Next: 6. Enumeration, descriptors, and device identity](06-enumeration-and-identity.md)
