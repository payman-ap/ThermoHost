# Firmware packet encoding

## Implementation

- `firmware.c` encodes each MAX6675 reading into the 10-byte frame specified
  in [protocol.md](protocol.md).
- Temperature is calculated from integer sensor counts using `counts * 25`.
- Bytes 2–5 hold temperature hundredths; bytes 6–9 hold the 32-bit device
  timestamp. Both are explicitly written in little-endian order.
- Invalid readings set flag bit 0 and write a zero temperature field.
- Frames are currently printed as hexadecimal text over USB CDC for inspection.
  This is not yet binary transport through a vendor WinUSB endpoint.

## Board-verified valid readings

The user compiled and flashed the firmware and supplied serial output:

```text
Raw: 0x02F8 | Temperature: 23.75 C
Packet: 01 00 47 09 00 00 61 44 00 00
```

- `0x00000947` = 2375 hundredths = 23.75 C.
- Timestamp `0x00004461` = 17505 ms.
- The following timestamp was 17755 ms, a 250 ms interval.
- Other supplied samples at 23.50, 23.25, and 23.00 C matched their encoded fields.

## Board-verified open thermocouple

The user disconnected a thermocouple lead and supplied:

```text
Raw: 0x7FFC | Thermocouple disconnected
Packet: 01 01 00 00 00 00 60 1A 00 00
Raw: 0x7FFC | Thermocouple disconnected
Packet: 01 01 00 00 00 00 5A 1B 00 00
```

- The fault flag is set and the temperature field is zero.
- Timestamps are 6752 and 7002 ms, again 250 ms apart.
- This matches the fault-frame format already tested by the DLL decoder.

## Next

Restore the thermocouple connection with USB disconnected. Then introduce the
TinyUSB vendor interface and Windows WinUSB binding in small steps. The current
CDC stdio configuration must be replaced when the custom descriptors are used.
