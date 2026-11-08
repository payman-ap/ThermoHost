# 16. Parsing fragmented and stale responses

## A parser is a small stateful machine

[FreshResponse.h](../../ThermoHost/ThermoDevice/FreshResponse.h) contains the
same parser used by real USB reads and host-only tests. It keeps a 32-byte
array and a count. `Feed` accepts one byte plus the current token, status
storage, and output pointer. Its boolean return means “a current candidate
response produced a final status,” not “this byte was valid.”

Keeping the parser outside the USB-specific translation unit makes testing
possible without hardware. Its header still includes the Windows-oriented API
declarations, so the supplied test script uses MSVC. “No hardware required”
does not automatically mean “portable to every compiler without adaptation.”

## First fill a candidate window

```cpp
Bytes[Count++] = Byte;
if (Count != sizeof(Bytes))
    return false;
```

Until thirty-two bytes are available, no complete candidate exists. The count
is retained across API calls and timeouts. The code writes at the current
position, increments, and then checks completeness.

The important invariant is that a call begins with `Count` between zero and
31. A successful candidate resets it to zero; a rejected full candidate
reduces it to 31. Therefore the next append fits. This reasoning is more useful
than merely observing that the test did not crash.

## Slide by one byte when identity does not match

```cpp
if (std::memcmp(Bytes, THERMO_RESPONSE_MAGIC, 4) != 0 ||
    std::memcmp(Bytes + 16, pToken, THERMO_TOKEN_SIZE) != 0)
{
    std::memmove(Bytes, Bytes + 1, --Count);
    return false;
}
```

The candidate must have both the prefix and the current token. If either
fails, the parser discards exactly one byte. It retains thirty-one because a
valid frame could begin inside the rejected candidate. Discarding the entire
buffer could lose a valid start after one noise byte.

For example, prepend byte `99` to a valid response. The first 32-byte window
contains `99` plus only 31 response bytes, so it fails. After shifting and
accepting the final byte, the window contains exactly the response. This is
resynchronization without an assumption about USB packet boundaries.

The cost is repeated comparison and shifting. At this thermometer's rate,
that is simple and acceptable. A high-rate protocol might use a state machine
or ring buffer that avoids shifting most bytes. Performance improvements must
preserve behavior under arbitrary fragmentation and noise.

## Matching identity is not enough

After a match, the count resets. The parser initializes status to InvalidPacket
and checks reserved flag bits and bytes. If they are nonzero, it returns true
with that status. Next it initializes SensorFault and tests flag bit zero.
Only a matching frame with valid reserved values and no sensor fault proceeds
to numeric decoding.

Resetting the count before these checks means the matching malformed or fault
frame is consumed as a complete result. It is not left in the rolling window
to accidentally influence the next read.

The checks are limited to the protocol rules implemented. The parser does not
test a physical temperature range, timestamp monotonicity, calibration, or a
payload checksum. It accepts any signed 32-bit temperature field after the
checked framing, token, and flags conditions.

## Decode signed fixed point explicitly

The four bytes at offset eight are widened to `uint32_t` before shifting.
The OR expression reconstructs the full bit pattern. The parser places that
unsigned value in an `int64_t`. If bit 31 is set, it subtracts `0x100000000LL`,
which is 2^32.

Why widen first? A signed 64-bit integer can represent both the unsigned
32-bit bit pattern and the desired negative result. The subtraction is
well-defined arithmetic rather than relying on implementation-specific
interpretation of an out-of-range signed cast.

Finally it divides by 100.0, writes the output, sets Ok, and returns true.
Temperature bytes from a fault frame never reach this final write.

## Walk through a timeout boundary

Suppose call A leaves seventeen bytes of its response in the parser when it
times out. Call B uses another token. The rest of A's response then arrives.
The completed A window has the wrong token for B, so it slides. Eventually a
complete B response arrives and is found. The parser need not clear its count
on every call to reject old data.

This persistence is intentional. Clearing the parser alone would not empty
all OS and device transport buffers anyway. Correlation handles history at
the protocol level. A random collision is theoretically possible, so the
guarantee is probabilistic with negligible accidental token-match likelihood,
not a mathematical claim that collisions cannot exist.

## Look at the tests as executable arguments

[fresh_response_test.cpp](../../tests/fresh_response_test.cpp) first supplies an
old token and verifies no output is published. It then adds prefix noise,
feeds a current response one byte at a time, and checks that completion occurs
only on the final byte. It tests synthetic negative decoding, fault flags,
reserved flags, and a partial old response followed by a new request.

These tests demonstrate important properties of the parser. They do not prove
the firmware always generates a fresh conversion, USB binding works, or every
possible byte sequence is safe. A test suite is evidence for its cases and
invariants, not a substitute for thinking about untested boundaries.

## Check your understanding

1. Prove that `Count` is below 32 when a normal `Feed` call begins.
2. Why retain 31 bytes instead of discarding a rejected candidate?
3. Why does the negative-value test not establish negative sensor capability?
4. Which field travels in the response but is not inspected by this parser?

Answers: [Chapter 16](24-answers.md#chapter-16).

<!-- NAVIGATION -->

---

[Previous: 15. The host read: randomness, writes, and time budgets](15-host-read-and-timeouts.md) · [Contents](README.md) · [Next: 17. The console: loading, looping, and stopping](17-console-loading-and-lifetime.md)
