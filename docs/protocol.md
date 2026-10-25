# Fresh-read protocol (version 2)

DLL API version 3 uses this protocol with `thermometer_usb`. The exported C
function signatures are unchanged. [Version 1](protocol_v1.md) documents the
previous streaming exercise, preserved by tag `live-thermometer-winusb` and
branch `packet-playground` for the earlier packet exercises.

## Compatibility and ownership

The vendor interface GUID, VID/PID and endpoints are unchanged. An IN vendor
control request (`bmRequestType=0xC0`, request `0x22`, value/index zero, length 5)
returns `54 48 02 14 20` (TH, version 2, request size 20, response size 32).
The DLL checks this at open and returns UnsupportedDevice (7) for old firmware.
Only one client may open the device; a sharing violation returns DeviceBusy (9).

## Command on bulk OUT 0x01

| Offset | Bytes | Meaning |
| --- | --- | --- |
| 0 | 4 | ASCII `THQ2` |
| 4 | 16 | Opaque request token |

The DLL generates a fresh random token for each read. The device restarts the
MAX6675 conversion after receiving the command, waits 250 ms while servicing
USB, then reads the sensor. A later complete command supersedes a pending one.
No measurements are transmitted while idle.

## Response on bulk IN 0x81

| Offset | Bytes | Meaning |
| --- | --- | --- |
| 0 | 4 | ASCII `THR2` |
| 4 | 1 | Flags: bit 0 thermocouple open; other bits zero |
| 5 | 3 | Reserved, zero |
| 8 | 4 | Signed two's-complement temperature x100, little endian |
| 12 | 4 | Device uptime at sensor read, milliseconds, little endian |
| 16 | 16 | Exact request token echoed |

Native structures are never serialized. The shared constants are in
`protocol/thermo_wire.h`. The DLL accepts only a response with the current
request token, retains partial frames, and skips old responses or framing noise.
A timed-out request's late reply cannot satisfy the next read except for the
negligible possibility of a 128-bit random-token collision. This token is
correlation data, not authentication.

A read has one 1000 ms budget covering command write and response assembly.
Faults, invalid frames, timeout and USB errors leave caller output unchanged.
A matching frame with invalid reserved bits returns InvalidPacket (3); a fault
returns SensorFault (4). Timestamps are not yet exposed through the public API.
Serialize operations on each handle; do not close it during a read.

The bytewise parser favors simplicity at this sensor's low sample rate; this
is not a high-throughput streaming design. Timeout means no successful current
response, not a claim about the sensor's physical condition. Close/reopen after
transport errors; automatic reconnection is outside this initial phase.
