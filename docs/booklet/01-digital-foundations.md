# 1. Digital representation and communication

A computer exchanges bit patterns. To turn a pattern into information, sender
and receiver must agree on its representation and meaning. The same four bytes
could denote an integer, characters, a floating-point number, or unrelated
flags. Communication begins with that agreement, before any particular cable,
library, or operating system is chosen.

This chapter establishes the vocabulary used throughout the book. USB will
provide transport; an application protocol will assign meaning to its payload.

## Bits, integers, and hexadecimal

A bit has two values. An eight-bit byte has 256 patterns. An unsigned n-bit
integer represents values from zero through `2^n - 1`. A signed representation
assigns some patterns to negative numbers. The bits themselves do not specify
which interpretation is intended.

Hexadecimal is base sixteen. Each digit represents four bits, so two digits
describe one byte. `0x81` is the integer 129, with binary representation
`10000001`. The prefix `0x` belongs to source-code notation; it is not part of
the transmitted byte.

| Decimal | Hexadecimal | Binary |
| --- | --- | --- |
| 1 | `01` | `00000001` |
| 4 | `04` | `00000100` |
| 20 | `14` | `00010100` |
| 32 | `20` | `00100000` |
| 129 | `81` | `10000001` |

Notice that decimal 20 and hexadecimal `0x20` differ. Confusing them changes a
message length from twenty bytes to thirty-two. When reading a trace, establish
the notation before interpreting values.

## Reading weighted positions

In binary, positions from the right have weights 1, 2, 4, 8, and so on.
Thus `10000001` means `128 + 1`. In hexadecimal, `0x1234` means
`1 * 4096 + 2 * 256 + 3 * 16 + 4 = 4660`. This is the same positional-number
idea used in decimal, with a different base.

An eight-bit unsigned value stops at 255 because all eight weights sum to
255. In signed eight-bit two's complement, the same 256 patterns cover
-128 through 127. Pattern `FF` is therefore 255 under one interpretation and
-1 under the other. A cast changes a program's interpretation or conversion;
it does not ask the sender which meaning was intended.

This reasoning also makes byte-order mistakes visible. Little-endian bytes
`34 12` reconstruct 4660. Reading them in reverse significance gives
`0x3412 = 13330`. Both are ordinary positive numbers, so an endian error can
produce plausible data rather than a crash.

## Fields inside a word

Hardware often packs several fields into one integer. A **mask** selects bits;
a **shift** moves them into a convenient position. If a twelve-bit field
occupies positions 14 through 3, its value is `(word >> 3) & 0x0FFF`. Testing
bit 2 uses `word & 0x0004`. The first expression extracts a number; the second
detects whether a flag is set.

For example, `word = 0x0330` gives field value 102 after shifting and masking.
Changing the word to `0x0334` leaves that field unchanged but sets bit 2. A
receiver must therefore know both the numeric scale and the flag's meaning.
It cannot establish validity by inspecting the numeric field alone.

This pattern occurs in sensor words, USB descriptor attributes, endpoint
addresses, and operating-system flags. A familiar-looking hex value has no
universal meaning across those contexts.

## Byte order and bit order

A multibyte integer needs a byte-order rule. In little endian, the least
significant byte comes first. Decimal 2550 is hexadecimal `0x000009F6`; its
four-byte little-endian representation is `F6 09 00 00`. Reconstruct it as:

```text
value = 0xF6 + 0x09 * 256 + 0x00 * 65536 + 0x00 * 16777216
      = 246 + 2304 = 2550
```

Big endian would transmit `00 00 09 F6`. Neither sequence is inherently more
correct. Agreement is what matters.

Bit order concerns which bit shifts first within a serial transfer. A sensor
connection can shift each byte most-significant-bit first while an application
protocol orders integer bytes least-significant-byte first. These are separate
boundaries and separate decisions.

## Signed fixed point

A protocol can represent a physical quantity using an integer and a scale.
If temperature is stored in hundredths of a degree, integer 2550 means
25.50 degrees C. This is **fixed-point representation**. It avoids transmitting
a native floating-point object whose format and byte ordering would also need
specification.

For signed 32-bit two's-complement values, a negative integer x has unsigned
bit pattern `2^32 + x`. Thus -1025 becomes `0xFFFFFBFF`, transmitted little
endian as `FF FB FF FF`. A decoder can assemble an unsigned pattern, widen it
to a signed 64-bit integer, then subtract `2^32` when bit 31 is set.

Resolution is not accuracy. Encoding hundredths permits convenient units; it
does not establish that a sensor can resolve or accurately measure 0.01 C.
A sensor with 0.25 C steps still produces multiples of 25 in this format.

## Text and binary data

ASCII `T`, `H`, `Q`, `2` has byte values `54 48 51 32`. A C string literal also
has a trailing zero in memory. A four-byte protocol identifier excludes that
terminator unless the protocol explicitly includes it.

Similarly, printing the characters `F6` transmits two character bytes. Sending
binary value `0xF6` transmits one byte. A terminal hex dump is a readable
description of binary data, not the same transmitted representation.

Pointers are another important distinction. A pointer identifies storage in
one address space. Copying its bits to another computer does not make the
pointed-to object available there. Messages should contain defined values or
identifiers, not native process pointers or unspecified C structure layouts.

## Units and time

Always attach units to a rate: bits/s, bytes/s, samples/s, or scans/s. Eight
channels sampled at 20,000 scans/s produce 160,000 channel samples/s. Their
byte rate depends on stored bytes per sample. Decimal MB means 1,000,000 bytes;
MiB means 1,048,576 bytes.

A timestamp also needs a definition. Device uptime, calendar time, acquisition
start, acquisition end, and host receipt are different quantities. An unsigned
32-bit millisecond counter wraps after about 49.71 days. It does not identify
an arbitrary number of wraps or distinguish a reboot by itself.

For short elapsed intervals, unsigned subtraction is useful because arithmetic
wraps modulo `2^32`. This supports periodic checks across one counter wrap
when the interval assumptions hold. It is not a general clock-synchronization
scheme.

### Work a counter wrap explicitly

Suppose a millisecond counter was `0xFFFFFFF0` and later reads `0x00000022`.
There were sixteen ticks to wrap, then thirty-four more: elapsed time is
50 ms. Unsigned 32-bit subtraction gives the same answer modulo `2^32`.

The calculation assumes the observation interval is short enough that an
unknown extra full wrap is impossible. A device restart would create another
ambiguity. Arithmetic can handle a representation boundary; establishing that
the clock has remained continuous is a separate system assumption.

## Example: a sensor result becomes a message

ThermoHost's MAX6675 interface supplies raw word `0x0330` as an illustrative
input. Its temperature field contains 102 counts. At 0.25 C/count, that is
25.50 C, encoded as 2550 hundredths and bytes `F6 09 00 00`.

If bit 2 is set, the firmware treats the sensor result as invalid even though
the temperature bits still contain a number. The application response sets a
fault flag. This small example combines field extraction, scale, byte order,
and validity. The communication mechanism has not yet entered the calculation.

**Exercises.** Decode `C4 09 00 00` as a signed temperature x100. Explain why
`0x0334` must not be accepted merely because its temperature field looks
reasonable. Answers and further problems are in [Appendix A](appendix-a-exercises.md#chapter-1).

<!-- NAVIGATION -->

---

[Contents](README.md) · [Next: 2. USB architecture and transfer types](02-usb-architecture.md)
