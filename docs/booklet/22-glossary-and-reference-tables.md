# 22. Glossary and quick reference

Use this chapter after reading the narrative. Definitions here are short
reminders rather than substitutes for the walkthroughs.

## Representation and measurement

| Term | Meaning here |
| --- | --- |
| Bit | One binary digit. |
| Byte | Eight bits in this project's data representations. |
| Hexadecimal | Base-16 notation; two digits describe one byte. |
| Mask | A bit pattern used to select fields with operations such as AND. |
| Shift | Moving bit positions left or right, used in field extraction and encoding. |
| Little endian | Least-significant byte of a multibyte integer comes first. |
| MSB first | Most-significant bit is shifted first on the configured SPI transfer. |
| Two's complement | The signed representation used for the protocol's temperature field. |
| Fixed point | Integer plus an agreed scale; temperature x100 means hundredths of a degree. |
| Resolution | Smallest distinguishable digital step; 0.25 C for the sensor result. |
| Accuracy | Closeness to the physical value, not the number of printed digits. |
| Cold-junction compensation | Converter correction associated with thermocouple reference temperature. |
| Uptime | Time since device or host boot, not calendar time. |

## Firmware and USB

| Term | Meaning here |
| --- | --- |
| Firmware | Code running on the RP2040. |
| SPI | Clocked local serial connection between RP2040 and MAX6675. |
| CS | Chip select, controlled on GP1; high during conversion. |
| GPIO function | Pin routing to ordinary I/O or a hardware peripheral. |
| USB host | PC side that schedules bus transactions. |
| USB device | RP2040 runtime peripheral. |
| Endpoint | Device-side transfer source or sink. |
| Pipe | Host-side software access relationship to an endpoint. |
| Interface | Group of related USB functionality and endpoints. |
| Configuration | Selectable collection of interfaces. |
| Descriptor | Structured metadata describing device capabilities or identity. |
| Enumeration | Host discovery and configuration process after attachment. |
| Bulk | Transfer type used here for commands and responses; no reserved latency. |
| Control transfer | Endpoint-zero exchange used for descriptors and capability. |
| CDC | Standard class behind the original serial-terminal experiment. |
| Vendor class | Device-specific interface class, `0xFF` here. |
| BOS | Descriptor collection used here to advertise Microsoft OS 2.0 capability. |
| FIFO | First-in, first-out buffer; finite storage can cause backpressure. |
| Backpressure | Inability to accept more data immediately due to downstream capacity. |
| Flush | Ask the stack to submit queued bytes; not proof of host parsing. |

## Windows and architecture

| Term | Meaning here |
| --- | --- |
| DLL | Dynamically loaded Windows code module. |
| API | Functions and behavioral contract offered to callers. |
| ABI | Binary calling, naming, and representation contract. |
| C linkage | Exported function linkage requested by `extern "C"`. |
| Import library | Link-time metadata for functions implemented in a DLL. |
| Opaque handle | Reference whose internals clients do not inspect. |
| WinUSB | Microsoft user-mode USB API plus kernel driver infrastructure. |
| SetupAPI | Windows APIs used here to enumerate device interfaces and paths. |
| GUID | Identifier for a Windows interface family in this project. |
| Overlapped I/O | Windows asynchronous-I/O mechanism; handle capability is separate from per-call use. |
| RAII | Ownership tied to object lifetime; used by the temporary `unique_ptr`. |
| Correlation token | Identifier matching a response to a request; not authentication. |
| Latency | Elapsed time from a defined start event to a defined completion event. |
| Jitter | Variation in event timing or latency. |
| Freshness | Here, conversion starts after the matched request reaches firmware. |
| Deadline | Time by which an outcome is required. |
| WDF | Windows Driver Frameworks, a later driver-development model. |
| KMDF / UMDF | Kernel-mode / user-mode framework driver models. |

## Status values

| Value | Name | Interpretation |
| --- | --- | --- |
| 0 | Ok | Output temperature was published. |
| 1 | InvalidArgument | A checked required pointer is null. |
| 2 | OutOfMemory | Private allocation failed. |
| 3 | InvalidPacket | A matching current response has invalid checked flags/reserved bytes. |
| 4 | SensorFault | A matching response reports an open thermocouple. |
| 5 | DeviceNotFound | No first present matching interface was enumerated. |
| 6 | UsbError | Transport/setup failure or another failure mapped to this status. |
| 7 | UnsupportedDevice | Required interface, endpoints, or capability were not accepted. |
| 8 | Timeout | Current read did not finish under its waiting policy. |
| 9 | DeviceBusy | Exclusive open encountered a sharing violation. |

These are library statuses, not raw Windows `GetLastError` values. Several
different lower-level causes can map to the same public status.

## Sizes and units

| Quantity | Value | Do not confuse with |
| --- | --- | --- |
| SPI read | 16 bits | USB message size. |
| Sensor temperature field | 12 bits | A 12-bit USB endpoint. |
| Conversion allowance | 250 ms | USB transfer time. |
| Request | 20 bytes | Hexadecimal `0x20` (32). |
| Response | 32 bytes | Thirty-two readings. |
| Token | 16 bytes | Authentication key. |
| Bulk endpoint packet size | 64 bytes | Required application frame length. |
| Read waiting budget | 1000 ms | Hard wall-clock or open-operation bound. |
| DLL API | Version 3 | Wire protocol version 2. |

## Common misconceptions corrected

“The board sends whenever it wants” means it queues data; the host still
initiates bus transactions. “The driver is my DLL” overlooks Microsoft's
function driver. “The data is reliable” does not establish that it belongs to
the current request. “The output has two decimals” does not establish
hundredth-degree physical accuracy. “The read has a timeout” does not establish
hard real-time completion. Return to the relevant chapter whenever one of
these shortcuts hides an assumption.

<!-- NAVIGATION -->

---

[Previous: 21. Practical labs: turn explanations into evidence](21-practical-labs.md) · [Contents](README.md) · [Next: 23. Source atlas and primary reading](23-source-atlas-and-reading.md)
