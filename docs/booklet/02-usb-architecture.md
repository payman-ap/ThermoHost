# 2. USB architecture and transfer types

USB connects a **host**, which schedules communication, to **devices**, which
provide functions. A device may be a keyboard, instrument, speaker, or storage
unit. Hubs form a tree between the host and devices. The roles describe control
of communication, not which participant has useful data ready.

When a device application says it “sends,” it normally makes data available
for host-scheduled communication. It does not independently seize a USB 2.0
bus. This is why a queued device response and a completed host read are
different events. The introductory [TinyUSB USB concepts guide](https://docs.tinyusb.org/en/stable/reference/usb_concepts.html)
describes these roles and common device classes.

## Endpoints, pipes, and interfaces

An **endpoint** is a device-side source or sink for transfers. A **pipe** is
the host-side software relationship used to access an endpoint. An interface
groups related device functionality; a configuration specifies a collection
of interfaces. The host learns those relationships during enumeration.

Direction is always from the host's perspective. IN travels toward the host;
OUT travels away from it. In an endpoint address, bit 7 indicates direction.
Thus `0x81` and `0x01` identify endpoint number 1 in opposite directions. An
instrument might receive commands on OUT and supply measurements on IN.

Endpoint zero is the default control endpoint. It is available before normal
configuration and supports control exchanges in both directions. It is not
one of the data endpoints listed under an ordinary interface. Microsoft's
[endpoint and pipe explanation](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-endpoints-and-their-pipes)
provides the terminology used by host software.

## Four transfer types, four sets of tradeoffs

| Type | Main purpose | Typical design question |
| --- | --- | --- |
| Control | Describe, configure, and control a device. | What request and reply does this operation define? |
| Bulk | Reliable payload transport using available bus time. | How much buffering tolerates variable delivery delay? |
| Interrupt | Periodically serviced, bounded-size data. | What service interval does the endpoint require? |
| Isochronous | Periodic time-dependent payload. | How are missing data and clock relationships handled? |

Bulk transport detects link errors and supports retry behavior, but bandwidth
is not reserved. A delivered payload can still be too old for its application.
Isochronous service reserves periodic opportunities and has different retry
and delivery semantics. Neither choice proves that a desktop process meets a
hard deadline. See Microsoft's [bulk-transfer](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-bulk-and-interrupt-transfer)
and [isochronous-transfer](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/transfer-data-to-isochronous-endpoints)
explanations for these transport properties.

The word “interrupt” names a USB transfer type. It does not mean that an
application receives a CPU interrupt directly. Likewise “bulk” does not require
large application messages; a small command can use bulk endpoints.

## Messages are above packets

A bus transaction involves transport-level packets. A host I/O request can
cover multiple packets. An application message is whatever the application
protocol defines. These boundaries need not coincide.

For an 800-byte application block on a full-speed bulk endpoint with 64-byte
maximum payload packets, at least thirteen data packets are needed. It remains
one application block. Conversely, a host library might return a chunk that
contains partial messages or several messages. A parser must use its protocol's
framing rules rather than presume one API call means one message.

A 64-byte packet maximum, a 64-byte software FIFO, and a 64-byte application
frame describe three different objects. Equal sizes do not make them the same
layer. This distinction becomes essential when interpreting throughput and
partial reads.

## What crosses the wires?

USB 2.0 uses a differential D+/D- pair. These are not independent transmit and
receive wires as in a UART. The controller handles line encoding, packet
identifiers, synchronization, and error-check fields. Full-speed signaling
uses NRZI encoding and bit stuffing; application code supplies payload bytes,
not the complete electrical packet. A device's “token” in an application
protocol is unrelated to a USB bus token packet.

For the physical and packet layers, the vendor
[USB 101 introduction](https://www.infineon.com/dgdl/Infineon-AN57294_USB_101_An_Introduction_to_Universal_Serial_Bus_2.0-ApplicationNotes-v09_00-EN.pdf?fileId=8ac78c8c7cdc391c017d072d8e8e5256)
is useful supplementary reading. The hardware and stack remove this machinery
before a host application parses its measurement bytes. Adding a bus CRC to an
application array would therefore change the application protocol rather than
complete an otherwise unfinished USB packet.

## USB versions and actual speed

Keep specification family, signaling mode, connector, and useful throughput
separate. Type-C is a connector system. A device descriptor's USB revision
does not command a particular speed. A negotiated link cannot use a mode the
endpoint hardware does not support.

| Mode or family | Nominal rate | Important qualification |
| --- | --- | --- |
| Low speed | 1.5 Mbit/s | Does not support bulk endpoints. |
| Full speed | 12 Mbit/s | The native RP2040 mode used in our example. |
| USB 2.0 high speed | 480 Mbit/s | Requires high-speed-capable hardware. |
| USB 3.2 Gen 1 | 5 Gbit/s | Earlier names include USB 3.0 and 3.1 Gen 1. |
| USB 3.2 Gen 2 | 10 Gbit/s | A higher-rate 3.x mode. |
| USB 3.2 Gen 2x2 | 20 Gbit/s | Two-lane 3.x operation. |
| Original USB4 | 20 or 40 Gbit/s capabilities | Multi-protocol routed architecture. |
| USB4 version 2.0 | Up to 80 Gbit/s symmetric | Optional asymmetric 120/40 Gbit/s operation. |

The 3.x mappings are documented by the [USB-IF 3.2 terminology guide](https://www.usb.org/sites/default/files/usb_3_2_language_product_and_packaging_guidelines_final.pdf).
For USB4, see the [original system overview](https://www.usb.org/sites/default/files/D1T1-3%20-%20USB4%20System%20Overview.pdf)
and [version 2.0 announcement](https://www.usb.org/sites/default/files/2022-10/USB-IF%20USB%2080Gbps%20Announcement_FINAL_v2.pdf).
These are nominal capabilities, not guaranteed application throughput. Shared
resources, encoding, protocol overhead, and software reduce usable capacity.
Optional modes also require checking the exact host/device/cable combination.

For a full-speed device, a 40 Gbit/s host port does not make the device run at
40 Gbit/s. The PC can accommodate the slower device, but it cannot redesign
its USB controller. Changing a packet-size macro has the same limitation.

## Example: choosing an access model

A keyboard benefits from a standardized HID interface; a speaker from the
audio ecosystem; an experimental measurement device may use a private vendor
interface. A USB class defines common behavior understood by host software.
“Vendor class” means the private behavior needs its own contract; it does not
automatically mean WinUSB or require a custom kernel driver.

ThermoHost uses one vendor interface and two bulk endpoints. Its small command
and response fit within individual maximum-size payload packets, but the host
parser still handles fragmented delivery. Its sensor rate is far below the
link's capacity, illustrating why the fastest USB mode is not always the main
engineering concern.

**Exercises.** Explain the directions of `0x01` and `0x81`. Distinguish an
application message, USB packet, and host read buffer. See
[Appendix A](appendix-a-exercises.md#chapter-2).

<!-- NAVIGATION -->

---

[Previous: 1. Digital representation and communication](01-digital-foundations.md) · [Contents](README.md) · [Next: 3. Enumeration, descriptors, and identity](03-enumeration-and-descriptors.md)
