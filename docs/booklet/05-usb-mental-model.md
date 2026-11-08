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

## Check your understanding

1. Explain both `0x81` and `0x01` from the host's perspective.
2. Why does a full 64-byte FIFO not mean a 64-byte sensor measurement?
3. Why can USB retry machinery coexist with stale application responses?
4. Does changing from CDC to vendor USB necessarily require different hardware?

Answers: [Chapter 5](24-answers.md#chapter-5).

<!-- NAVIGATION -->

---

[Previous: 4. SPI: from pins to a sensor word](04-spi-from-pins-to-word.md) · [Contents](README.md) · [Next: 6. Enumeration, descriptors, and device identity](06-enumeration-and-identity.md)
