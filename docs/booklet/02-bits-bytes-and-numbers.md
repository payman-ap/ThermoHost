# 2. Bits, bytes, hexadecimal, and units

## Start with representation

A bit has two possible values. A byte in this project has eight bits, so an
unsigned byte represents integers from 0 to 255. A 16-bit unsigned value has
65,536 possible bit patterns; a 32-bit value has 4,294,967,296. The bits do not
carry their own units. `250` might mean milliseconds, counts, bytes, or hundredths
of a degree. The surrounding contract supplies the meaning.

Hexadecimal is convenient because one hex digit represents four bits. Thus
`0x81` is `1000 0001` in binary: bit 7 and bit 0 are set. The `0x` prefix belongs
to source-code notation. Those two characters are not sent over the wire when
the program transmits the single byte value `0x81`.

| Decimal | Hexadecimal | Eight-bit binary |
| --- | --- | --- |
| 0 | `00` | `00000000` |
| 1 | `01` | `00000001` |
| 4 | `04` | `00000100` |
| 32 | `20` | `00100000` |
| 129 | `81` | `10000001` |
| 255 | `FF` | `11111111` |

Notice the distinction between decimal 20 and hex `0x20`: the latter is decimal
32. The capability reply contains request size `0x14` and response size `0x20`,
which mean 20 and 32 bytes. Misreading these as decimal values makes a valid
protocol look contradictory.

## Masks and shifts in the actual sensor code

The live firmware uses:

```c
bool valid = (raw & 0x0004u) == 0u;
uint32_t counts = (raw >> 3) & 0x0FFFu;
```

The first expression isolates bit 2. Bitwise AND retains a bit only where both
operands have a 1. If that bit is set, the reading is marked invalid. The `u`
suffix makes the literal unsigned. The second expression removes three low
bits, then retains twelve bits. This is field extraction: a structured word
becomes an ordinary integer count.

For a worked example, suppose the two SPI bytes are `03 30`. Reconstructing them
in sensor order gives `raw = 0x0330 = 816`. Shifting right three places divides
this nonnegative integer by eight and discards the remainder: 102 remains.
The mask retains that value. Each count means 0.25 degrees, so the temperature
is 25.50 degrees. The wire field uses `102 * 25 = 2550` hundredths of a degree.

If the raw word were `0x0334`, the temperature bits would still contain 102,
but bit 2 would be set. The code deliberately sends zero in the temperature
field and sets the fault flag. You must inspect validity before treating any
numeric field as a measurement.

## Byte order is a contract

The sensor sends its high byte first. The application protocol sends multibyte
integers low byte first. These are separate choices at separate boundaries.
“SPI is MSB-first” describes bit order during shifting; “little endian” describes
the ordering of bytes in an integer field. Neither implies the other.

For 2550, hexadecimal is `0x000009F6`. Little-endian transmission is:

```text
offset:     8   9  10  11
byte:      F6  09  00  00
weight:     1 256 65536 16777216
```

The decoder computes `0xF6 + (0x09 << 8) = 246 + 2304 = 2550`. Dividing by
100.0 yields 25.5. The `.0` matters: integer division would discard the fraction.
Your encoder uses explicit shifts instead of copying a native C structure,
so the contract does not depend on compiler padding or host byte order.

## Signed values and two's complement

The protocol permits signed 32-bit temperature values. A negative test value
does not mean the MAX6675 measures negative temperatures in this setup. It
tests the general decoder's signed representation.

For -10.25 degrees, `temperature_x100 = -1025`. Its 32-bit two's-complement
pattern is `2^32 - 1025 = 0xFFFFFBFF`; little-endian bytes are `FF FB FF FF`.
The parser first assembles an unsigned value, then subtracts `2^32` in a wider
signed type if bit 31 is set. This makes the interpretation explicit instead
of relying on an out-of-range unsigned-to-signed cast.

Unsigned timestamps wrap. A 32-bit millisecond uptime rolls over after about
49.71 days. It is not a calendar time. Subtracting unsigned timestamps modulo
`2^32` is useful for short elapsed intervals, as the probe does, but does not
identify an arbitrary number of wraps or distinguish a reboot by itself.

## Text is another encoding

ASCII `T`, `H`, `Q`, `2` corresponds to bytes `54 48 51 32`. The string literal
`"THQ2"` in C also has a terminating zero in storage, but the protocol copies
only four characters. Sending `sizeof("THQ2")` would send five bytes and shift
the token out of place.

Likewise, printing `F6` to a terminal transmits characters representing a hex
number. It does not transmit the single binary byte `0xF6`. This is why the
original CDC demonstration and the binary WinUSB protocol are not equivalent
despite showing the same apparent numbers.

## Precision without invented accuracy

The wire stores hundredths of a degree because it is a simple fixed-point
format. Sensor results remain multiples of 25 in that field. Printing `25.50`
does not create a 0.01-degree sensor. Fixed-point scaling is a representation
decision; physical resolution and calibration are separate properties.

Integer arithmetic is especially helpful here: every quarter degree converts
exactly to 25 hundredths. No floating-point operation is needed on the board to
construct a packet. The Windows API can still expose a convenient `double`.

## Check your understanding

1. Decode bytes `C4 09 00 00` as little-endian temperature x100.
2. Extract the temperature and fault state from raw `0x0554`.
3. Why is transmitting a C string with its zero terminator wrong for `THQ2`?
4. Explain why 32 response bytes do not mean 32 temperature measurements.

Answers: [Chapter 2](24-answers.md#chapter-2).

<!-- NAVIGATION -->

---

[Previous: 1. The complete measurement](01-complete-measurement.md) · [Contents](README.md) · [Next: 3. Hardware and the meaning of a measurement](03-hardware-and-measurement.md)
