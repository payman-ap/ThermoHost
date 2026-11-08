# From thermocouple to Windows: the ThermoHost booklet

A practical course in bytes, SPI, USB devices, WinUSB, native DLLs, and the
engineering of timely measurements.

**Read in order the first time.** This booklet assumes you can read simple C
functions but have not written a USB driver. Each chapter introduces a small
set of ideas, follows them into this repository, and finishes with questions.
Worked answers and a sequence of practical labs are included. You do not need
to change the working firmware to understand the explanations.

This edition follows the root-level `ThermoHost/` layout, DLL API **3**, and
wire protocol **2**, inspected on **8 November 2026**. The sensor is a MAX6675
connected to a Waveshare RP2040-Zero. Firmware is C11; the Windows projects are
C++17 using MSVC v142. Later changes may alter code excerpts. Links point to
source files rather than frozen line numbers so they work in GitHub.

## Reading routes

For a full course, follow the numbered contents below. For a quick explanation
of a measurement, read chapters 1, 10, 11, 15, and 16. For the Windows side,
read chapters 7 and 13 through 17. For the USB device side, read chapters 5, 6,
and 9 through 12. Return to chapter 19 before using this design as the basis of
a time-sensitive application.

The expanded edition adds a second level of study. Chapter 5 now separates
USB generations, actual link modes, connectors, and payload rates. Chapter 7
develops WinUSB from its historical motivation through instrument examples.
Chapter 8 identifies the authors and roles of the SDKs, libraries, and tools.
After the core course, use these advanced lectures:

- [Chapter 25](25-linux-and-portability.md): Linux equivalents and a proposed
  portable architecture. Read after chapters 7 and 14.
- [Chapter 26](26-capacity-and-bottleneck-calculations.md): demand, overhead,
  throughput, bottlenecks, and buffers calculated by hand. Read after chapter 19.
- [Chapter 27](27-designing-continuous-streams.md): continuous acquisition,
  asynchronous buffer ownership, clock drift, and stream contracts.

The labs and reference chapters retain their original filenames so existing
GitHub links remain useful. The advanced lectures have additional problems
and [worked answers](24-answers.md#chapter-25).

The manuscript is intended for booklet-scale study, roughly the requested
50–150 printed-page range depending on font, code wrapping, and figure size.
Markdown has no fixed pages; the word count and reading estimate are reported
in [the publication notes](publication-notes.md). The goal is a useful course,
not padding to an artificial page count.

## Contents

<!-- CONTENTS -->

1. [The complete measurement](01-complete-measurement.md)
2. [Bits, bytes, hexadecimal, and units](02-bits-bytes-and-numbers.md)
3. [Hardware and the meaning of a measurement](03-hardware-and-measurement.md)
4. [SPI: from pins to a sensor word](04-spi-from-pins-to-word.md)
5. [USB without the mystery](05-usb-mental-model.md)
6. [Enumeration, descriptors, and device identity](06-enumeration-and-identity.md)
7. [What WinUSB is and how it works](07-what-winusb-is.md)
8. [Building, flashing, linking, and loading](08-build-flash-and-load.md)
9. [Three firmware applications as controlled experiments](09-three-firmware-walkthroughs.md)
10. [The live firmware as a state machine](10-live-firmware-state-machine.md)
11. [The wire protocol, byte by byte](11-wire-protocol-workshop.md)
12. [Control requests and the Microsoft descriptors](12-control-and-os-descriptors.md)
13. [DLL design, C linkage, and ownership](13-dll-and-ownership.md)
14. [Discovering and opening the Windows device](14-windows-device-discovery.md)
15. [The host read: randomness, writes, and time budgets](15-host-read-and-timeouts.md)
16. [Parsing fragmented and stale responses](16-parser-and-fragmentation.md)
17. [The console: loading, looping, and stopping](17-console-loading-and-lifetime.md)
18. [Testing the boundaries and diagnosing failures](18-testing-and-diagnosis.md)
19. [Real-time thinking: latency, jitter, freshness, and queues](19-real-time-and-buffering.md)
20. [From this project toward device-driver development](20-toward-device-drivers.md)
21. [Practical labs: turn explanations into evidence](21-practical-labs.md)
22. [Glossary and quick reference](22-glossary-and-reference-tables.md)
23. [Source atlas and primary reading](23-source-atlas-and-reading.md)
24. [Worked answers and reasoning checks](24-answers.md)
25. [Linux, libusb, and a portable thermometer library](25-linux-and-portability.md)
26. [Calculating capacity and finding the bottleneck](26-capacity-and-bottleneck-calculations.md)
27. [Designing continuous streams from first principles](27-designing-continuous-streams.md)

<!-- END CONTENTS -->

## How to study

Keep the matching source file open beside the chapter when possible. Trace one
successful call before tracing failures. For each code fragment, ask: who owns
this memory, what units does this number use, when can this operation finish,
and which side of the USB connection executes it? These questions expose more
mistakes than memorizing function names.

Quoted code is an excerpt of existing repository code unless explicitly marked
as pseudocode or an exercise. Ellipses in an excerpt indicate omitted code, not
compilable statements. Exercises propose changes in a separate experimental
branch; they are not changes made by this booklet. All graphs are explanatory
models, not measurements from the connected board.

## Important distinctions to keep nearby

| Terms | Difference in this project |
| --- | --- |
| DLL API 3 / wire protocol 2 | Application-facing function contract / bytes exchanged over USB. |
| `firmware.uf2` / `thermometer_usb.uf2` | CDC terminal experiment / current vendor-USB thermometer. |
| WinUSB / ThermoDevice | Microsoft's USB access system / your project-specific wrapper. |
| USB packet / application frame | Transport-level unit / your 20- or 32-byte message. |
| Resolution / accuracy | Smallest representable sensor step / closeness to physical truth. |
| Fresh / instantaneous | Conversion begins after a request / result exists without delay. |
| Timeout / hard deadline | Software stops waiting / completion is guaranteed by a bound. |

## Reproducing the booklet

The [asset generator](assets/generate-assets.ps1) creates the PNG illustrations
using Windows System.Drawing. The [booklet checker](tools/check_booklet.py)
checks links, navigation, code excerpts, and worked byte examples using only
Python's standard library. Images live in `assets/` and use relative Markdown
links, so GitHub can display them on a tablet without a special viewer.

The [calculation checker](tools/check_calculations.py) prints the worked rate,
latency, queue, and clock-drift examples. It uses exact rational arithmetic to
check the selected results and requires no third-party Python packages:

```powershell
python docs/booklet/tools/check_calculations.py
```

Start with [Chapter 1: the complete measurement](01-complete-measurement.md).
