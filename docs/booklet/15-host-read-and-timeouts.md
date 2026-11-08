# 15. The host read: randomness, writes, and time budgets

## Validate before touching the device

`Thermo_ReadTemperature` begins by rejecting null handle and output pointers.
The function does not write a speculative temperature before USB work. On
failure, the caller's existing value remains intact. This is a small but useful
API guarantee: the status tells the caller whether a new value was published.

This validation only establishes nonnull arguments. The handle must still be
a live object from `Thermo_Open`, and the output pointer must refer to writable
storage. The DLL cannot make a dangling or arbitrary nonnull pointer safe by
checking it against zero.

## Create a request belonging to this call

```cpp
std::uint8_t Request[THERMO_REQUEST_SIZE] = {};
std::memcpy(Request, THERMO_REQUEST_MAGIC, 4);
if (BCryptGenRandom(0, Request + 4, THERMO_TOKEN_SIZE,
    BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0)
    return ThermoStatus_UsbError;
```

The request array lives for the duration of the read function. The first four
bytes identify the protocol command; the last sixteen are generated using the
Windows system-preferred random generator. Failure to generate a token stops
the read before sending anything.

The token is not a C string, so zero bytes are allowed. Comparing it requires
`memcmp` with sixteen bytes, not `strcmp`. A string comparison could stop early
and falsely accept a mismatched token containing a zero.

The response parser is persistent in the device object, while this request is
local. Each feed operation receives the current token. Thus bytes retained
from a previous timed-out call are tested against the new request's identity.

## One overall budget, decreasing per-call waits

```cpp
const ULONG TimeoutMs = 1000;
const ULONGLONG Started = GetTickCount64();
```

The function records a monotonic-style uptime counter after constructing the
request. It sets the OUT pipe timeout to 1000 ms and writes the command. A
failed write maps `ERROR_SEM_TIMEOUT` to Timeout and other failures to UsbError.
A successful short write is also UsbError because the complete command was
not transferred as expected.

After writing, each loop iteration calculates elapsed time. If the total is
already at least 1000 ms, it returns Timeout. Otherwise it sets the IN pipe's
timeout to the remaining budget before the next read. The intent is to avoid
granting another full second for every byte or every discarded old frame.

For example, if writing takes 40 ms and stale-data processing takes another
100 ms, the next read gets roughly 860 ms rather than 1000. One response has
one budget. This accounting is different from configuring a per-call timeout
once and then executing an arbitrarily large number of calls.

## Budget does not equal hard wall-clock guarantee

Windows can deschedule the thread, and pipe requests have their own queueing
and completion semantics. `WinUsb_SetPipePolicy` and other operations take
time too. A transfer timeout is an I/O policy, not proof that the entire C++
function returns by an exact external deadline. Microsoft's
[policy API](https://learn.microsoft.com/en-us/windows/win32/api/winusb/nf-winusb-winusb_setpipepolicy)
and [policy behavior](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/winusb-functions-for-pipe-policy-modification)
should be read with this distinction in mind.

The code's 1000 ms budget covers the command/response portion after token
creation. It does not cover `Thermo_Open`, including the capability control
transfer. Do not assume every exported operation in the library has a shared
one-second limit.

## Read bytes into a persistent parser

```cpp
UCHAR Byte = 0;
ULONG Received = 0;
if (!WinUsb_ReadPipe(pHandle->UsbInterface, pHandle->BulkInPipe,
    &Byte, 1, &Received, 0))
```

The requested buffer length is one byte and the final null argument selects
synchronous use. The actual transferred count is checked before feeding the
byte. A successful API result with zero bytes does not mean a complete frame
or valid temperature exists.

With partial reads enabled and automatic flushing disabled, the rest of a
larger received packet is retained for subsequent reads. The
[ReadPipe reference](https://learn.microsoft.com/en-us/windows/win32/api/winusb/nf-winusb-winusb_readpipe)
describes the buffer, count, endpoint, and overlapped arguments. The particular
one-byte strategy is your wrapper's choice.

The parser returns false while searching or assembling. The read loop
continues and rechecks the time budget. When the parser reports a completed
current response, its status becomes the function's result. A current sensor
fault is therefore returned promptly as a result rather than ignored until
timeout.

## What is skipped and what ends the call?

| Event | Current read behavior |
| --- | --- |
| Noise or nonmatching prefix | Continue scanning until budget expires. |
| Response with another token | Skip it and continue. |
| Matching response with invalid reserved bits | Return InvalidPacket. |
| Matching fault response | Return SensorFault without updating temperature. |
| Matching valid response | Decode, publish temperature, return Ok. |
| USB read failure | Return Timeout or UsbError. |

An arbitrary invalid byte stream can thus produce Timeout rather than
InvalidPacket. InvalidPacket specifically results from a frame that matches
the expected identity but violates the checked content rules. That distinction
helps you interpret negative tests correctly.

## Output publication is the final step

Only the parser's valid-data path writes the caller's `double`. The transport
code never exposes a partially assembled numeric field. This resembles a
small transaction: collect and validate privately, then publish once.

For a future richer result, the same pattern could fill a local result
structure and copy it to caller storage only after every required field is
validated. The current API returns only temperature; flags become statuses,
and the timestamp remains internal to the frame.

## Check your understanding

1. Why would a 1000 ms timeout on each of 32 reads be a poor overall policy?
2. Why can arbitrary garbage cause Timeout rather than InvalidPacket?
3. Does the read budget apply to capability checking in `Thermo_Open`?
4. Why must a token be compared as bytes rather than as text?

Answers: [Chapter 15](24-answers.md#chapter-15).

<!-- NAVIGATION -->

---

[Previous: 14. Discovering and opening the Windows device](14-windows-device-discovery.md) · [Contents](README.md) · [Next: 16. Parsing fragmented and stale responses](16-parser-and-fragmentation.md)
