# Thermometer wire protocol, version 1

## Purpose

Define the bytes exchanged between firmware and the Windows DLL independently
of the microcontroller. The DLL converts the fixed-point temperature to the
public API's `double` in degrees Celsius.

This protocol is specified but not yet implemented or tested over USB.

## Correction to the original roadmap

The roadmap proposed an 8-byte frame with signed 16-bit `temp_x100`.
That field can represent only -327.68 through 327.67 degrees C and cannot
cover the MAX6675's full positive measurement range.

Use signed 32-bit `temp_x100` instead. The resulting frame is **10 bytes**.
There is no requirement for this application frame to have a power-of-two size.
Any later implementation for Project 1 must use this same format to share the DLL.

## Temperature frame

All multibyte integers are **little endian** (least significant byte first).
Signed integers use two's-complement representation.

| Byte offset | Size | Field | Meaning |
| --- | --- | --- | --- |
| 0 | 1 | `type` | `0x01`: version 1 temperature frame |
| 1 | 1 | `flags` | Bit 0: open thermocouple; other bits reserved, zero |
| 2 | 4 | `temp_x100` | Signed 32-bit temperature in hundredths of a degree C |
| 6 | 4 | `timestamp_ms` | Unsigned 32-bit milliseconds since device boot |

- With flags zero, `temp_x100 / 100.0` gives degrees Celsius.
- If bit 0 is set, the temperature is invalid. The firmware writes zero in
  `temp_x100`, and the receiver must report a sensor fault rather than 0 C.
- Unknown types or reserved flag bits must be treated as unsupported frames,
  rather than interpreted as valid temperatures.
- The timestamp wraps modulo 2^32 and starts over on reboot. It is not wall time.
- The MAX6675's quarter-degree counts convert exactly using `counts * 25`;
  floating-point arithmetic is not needed to construct its temperature field.
- Encode and decode byte arrays explicitly. Do not send a native C/C++ struct
  directly: compiler padding, alignment, and machine byte order are not the protocol.

## Examples

Valid 25.50 C reading, timestamp 1000 ms:

```text
01 00 F6 09 00 00 E8 03 00 00
```

Here, `0x000009F6` is 2550, and `0x000003E8` is 1000.

Open thermocouple, timestamp 1000 ms:

```text
01 01 00 00 00 00 E8 03 00 00
```

## Separation from the DLL API

An opaque device handle is a process-local pointer, never a wire field.
The DLL keeps USB handles and receive state private. Clients receive operation
status and decoded values, not firmware structs or raw USB handles.

The initial WinUSB transport will carry these frames from device to host.
USB descriptors, endpoint configuration, buffering, and timeout behavior are
subsequent implementation steps.
