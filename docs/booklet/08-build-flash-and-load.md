# 8. Building, flashing, linking, and loading

## Who supplies the software?

Before learning API names, identify who owns each layer. “The USB library” is
not one product. The board has a hardware SDK and embedded USB stack; Windows
has a native API and kernel driver; your repository supplies the instrument
protocol. The following inventory prevents several common confusions.

| Component | Producer or project | Runs where? | Role in ThermoHost |
| --- | --- | --- | --- |
| `ThermoConsole.exe`, `ThermoDevice.dll`, `FreshResponse.h` | This repository's author | Windows process | Client policy, thermometer API, transport orchestration, decoding. |
| `Winusb.dll`, `Winusb.sys`, WinUSB SDK declarations | Microsoft | Windows user/kernel layers | General USB device access. |
| SetupAPI and `Setupapi.lib` | Microsoft Windows SDK/system | Windows host | Device-interface discovery and paths. |
| BCrypt and `Bcrypt.lib` | Microsoft Windows SDK/system | Windows host | System random tokens through the native cryptography API. |
| MSVC v142 and MSBuild | Microsoft | Development PC | Compile/link and build the Windows targets. |
| `pico_stdlib`, `hardware_spi`, GPIO/time APIs | Raspberry Pi Pico SDK | Linked into RP2040 firmware | Hardware and runtime services. |
| `tinyusb_device` and `tusb.h` | TinyUSB open-source project | Linked into RP2040 firmware | USB device core, classes, and controller integration. |
| libusb | Independent open-source libusb project | Host user space | A potential cross-platform host API; not currently used by ThermoHost. |
| CMake | Kitware-originated open-source project | Build machine | Generate/configure build rules from target descriptions. |
| Ninja | Open-source Ninja project | Build machine | Execute generated build rules efficiently. |
| GCC / Arm-target cross compiler | GNU compiler project, distributed in an Arm-target toolchain | Build machine | Produce machine code for the board. |

The [WinUSB architecture reference](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/winusb-architecture)
identifies Microsoft's components. Raspberry Pi publishes the
[Pico SDK](https://github.com/raspberrypi/pico-sdk), including hardware APIs and
integration with third-party libraries. TinyUSB is a separate project with
Ha Thach credited in its [license](https://github.com/hathach/tinyusb/blob/master/LICENSE)
and contributions documented in its [repository](https://github.com/hathach/tinyusb).
Bundling TinyUSB in an SDK does not make its upstream authorship Microsoft or
Raspberry Pi. The [libusb author record](https://github.com/libusb/libusb/blob/master/AUTHORS)
likewise identifies a community project rather than a Microsoft system DLL.

The build tools have different jobs too. [CMake](https://cmake.org/about/)
describes/generates the build; [Ninja](https://ninja-build.org/) executes its
generated rules; [GCC](https://gcc.gnu.org/) compiles source. A downloaded
toolchain package can combine programs from multiple upstream projects.

### A worked ownership question

Suppose `spi_read_blocking` produces the wrong raw word. Investigate wiring,
sensor timing, SDK configuration, and the SPI peripheral. Microsoft WinUSB
cannot repair that word after acquisition. Suppose a correct raw word becomes
wrong temperature bytes: investigate your encoder. Suppose correct bytes reach
the host but are rejected: inspect your parser and protocol contract.

Knowing provenance does not mean assuming vendor software is perfect. It
means locating the appropriate source, documentation, version, and boundary
for evidence. The installed TinyUSB version, the configured Pico SDK version,
and Windows API availability are separate facts.

### Headers, import libraries, and runtime implementations

`#include <winusb.h>` exposes declarations to the compiler. `Winusb.lib` tells
the linker how to import system functions. `Winusb.dll` implements their
user-mode entry points at runtime, and `Winusb.sys` participates below them.
Installing a header alone does not install an operating-system driver binding.

On the board, linking `tinyusb_device` usually places the relevant machine code
inside the firmware image. There is no TinyUSB DLL to load from the RP2040's
filesystem. This contrast is a useful exercise in static linkage versus host
dynamic linkage: similar source-level function calls can have different
deployment models.

## Two targets, two toolchains

The firmware build targets Arm code and links Pico SDK libraries. The Windows
solution targets the PC and links Windows SDK libraries. A successful build on
one side does not establish compatibility with the other. Their shared
contract is the protocol, not a common executable file.

Open [firmware/CMakeLists.txt](../../firmware/CMakeLists.txt). It specifies C11,
C++17, the `waveshare_rp2040_zero` board, and Pico SDK initialization. The SDK
import precedes `project(...)` so the cross-compilation environment can be
established appropriately. Extension-managed settings select the local SDK and
toolchain; machine-specific paths are not portable project requirements.

## Understand a target

```cmake
add_executable(thermometer_usb
    thermometer_usb.c
    usb/usb_descriptors.c
    usb/winusb_descriptors.c
)
```

This target combines acquisition/control logic and two descriptor translation
units into one firmware application. `target_include_directories` adds the
`usb/` directory so the target finds `tusb_config.h`. Linking `hardware_spi`
supplies SPI APIs, `tinyusb_device` supplies device USB support, and
`pico_unique_id` supports the serial string. `pico_stdlib` supplies common SDK
facilities.

The original `firmware` target instead enables SDK USB stdio. The vendor targets
disable that console interface because they provide their own USB setup.
Enabling stdio USB is not a harmless way to add debug printing to this custom
descriptor arrangement; a combined device needs deliberate configuration.

## A compile definition changes firmware behavior

```cmake
target_compile_definitions(thermometer_usb PRIVATE THERMO_FRESH_READS=1)
```

This enables the fresh-protocol control response in a source file shared with
the probe. `PRIVATE` means the definition applies to compilation of this target.
The same descriptor source therefore compiles differently for the two images.
Merely comparing filenames is insufficient to determine the final behavior.

The capability response still contains literal sizes in the descriptor source,
while the shared protocol header defines sizes elsewhere. They currently agree.
A future protocol edit must keep both consistent or the DLL will reject the
firmware. This is a concrete example of duplicated contract data.

## Build and flash the current board

In the configured Pico SDK terminal, from the repository's `firmware` directory:

```powershell
cmake --build build --target thermometer_usb
```

Hold BOOT while connecting the board, release it, then copy
`firmware/build/thermometer_usb.uf2` to the RPI-RP2 drive. The bootloader writes
flash and restarts the application. The drive disappearing is expected; the
application then exposes the vendor interface, not a COM port.

The `.elf` includes information useful to debugging and tools. The `.bin` is a
raw binary image. The `.uf2` packages firmware for the bootloader workflow.
These are different outputs of the same target, not three applications that
must all be installed. To build the other experiments, replace the target with
`firmware` or `usb_probe`.

## Build the Windows solution

Open [ThermoHost.sln](../../ThermoHost/ThermoHost.sln). Select Debug and x64, then
build. The two projects use MSVC v142 and C++17. `ThermoDevice` is a dynamic
library; `ThermoConsole` is an application. Their relative project reference
remains valid within the root-level `ThermoHost/` folder.

The shared protocol header appears as `../../protocol/thermo_wire.h` relative
to `ThermoDevice`. The test includes the parser from the root-level Windows
folder. These are source-tree relationships. A runtime DLL lookup is a
different kind of path resolution.

Keep the freshly built `ThermoDevice.dll` beside `ThermoConsole.exe` in
`ThermoHost/x64/Debug/`. An old DLL elsewhere in the search path can otherwise
confuse investigation. The console prints the loaded API version, which is
useful evidence but does not fully identify the file's build provenance.

## Compile-time link versus runtime load

The linker resolves references while making an executable or library. Runtime
loading maps a DLL into the process and obtains function addresses while the
program runs. Microsoft distinguishes
[implicit and explicit linking](https://learn.microsoft.com/en-us/cpp/build/linking-an-executable-to-a-dll?view=msvc-170).

The current console uses explicit loading. It includes the header to obtain
matching types but invokes resolved pointers rather than direct imported
functions. A project dependency can still build the DLL first; it does not
change those calls into implicit imports.

`Winusb.lib`, `Setupapi.lib`, and `Bcrypt.lib` in the DLL build are import
libraries for Windows APIs. The project-specific `ThermoDevice.lib` is also a
link-time artifact, not a substitute for `ThermoDevice.dll` at runtime.

## Debugging evidence belongs to the right build

Record the commit, target, configuration, flash action, and loaded DLL version
when collecting results. A stale firmware binary can survive source edits.
A stale console executable can survive editing its source. A successful build
does not prove that the connected board runs that build.

Useful first runs from the Windows output directory are:

```powershell
.\ThermoConsole.exe --help
.\ThermoConsole.exe --samples 3
.\ThermoConsole.exe --samples 3 --interval-ms 3000
```

The help path checks argument handling without opening USB. The finite run
checks three attempts. The pause run checks behavior after idle time. None is
by itself a full fault-recovery or timing qualification.

## Check your understanding

1. Why does `THERMO_FRESH_READS` affect a shared source file differently for two
   targets?
2. What is the difference between a compiler include path and DLL search order?
3. Which artifact is copied in BOOT mode, and which runs on Windows?
4. Why is successful compilation insufficient evidence of a successful flash?

Answers: [Chapter 8](24-answers.md#chapter-8).

<!-- NAVIGATION -->

---

[Previous: 7. What WinUSB is and how it works](07-what-winusb-is.md) · [Contents](README.md) · [Next: 9. Three firmware applications as controlled experiments](09-three-firmware-walkthroughs.md)
