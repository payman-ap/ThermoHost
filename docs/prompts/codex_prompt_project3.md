# Prompt for Codex: guide me through Project 3 (RP2040-Zero + MAX6675 + WinUSB + DLL + Qt)

Copy everything below the line into Codex. The roadmaps are in `docs/roadmaps/`:
`project3_rp2040_zero_thermometer.md`, with Project 1 and Project 2 available for context.

---

## Who I am and what I want

I am a C++ developer with background in digital signal processing who uses Qt 5.15 with Visual Studio 2019 (MSVC) at my job to develop medical software. I am teaching myself embedded and Windows device-side programming. My long-term goal is a custom USB sound card (STM32 BlackPill, INMP441 microphone, later I2S playback) with my own Windows DLL and a Qt app. This project is the first step.

My learning goals, in order of importance:
1. Writing C++ DLLs (exports, calling conventions, C API, error handling, 32/64-bit, using them from Qt with `QLibrary`)
2. Talking to USB devices on Windows with WinUSB (SetupAPI, overlapped I/O, `winusb.h`) and understanding how Windows binds drivers (Microsoft OS 2.0 descriptors)
3. Embedded C/C++ on a microcontroller: SPI, USB device descriptors, memory handling (buffers, ring buffers, no dynamic allocation)
4. Protocol and API design that does not depend on the hardware
5. Qt 5.15 app that threads correctly and plots live data
6. Later: signal processing (not in this project)

I do not want a kernel-mode driver. By "driver" I mean user-mode code: firmware on the board plus my own DLL on top of the built-in WinUSB driver. No WDK and no test-signing mode.

## My hardware and environment

- Windows 11 work PC. Installed: PlatformIO, Visual Studio 2019, Qt 5.15. No STM32CubeIDE. I cannot easily use a VM. I can use a second laptop if really needed.
- RP2040-Zero board (I have 2; the flash size is 2 MB, please tell me how to check if needed)
- MAX6675 module with a working K-type thermocouple
- Other boards available (not for this project): WeAct BluePill, BlackPill F411, F446, ST-Link V2, 24 MHz logic analyzer, FT232RL and CP2102 USB-UART adapters
Also the complete list of my hardware is in:
Electronics_Inventory_AI_Export.csv

## The task

Read `docs/roadmaps/project3_rp2040_zero_thermometer.md` in this repository. It is my roadmap with six sessions. Work through it with me step by step, one session at a time.

## How I want you to work (important)

- This is a learning project. Do not just dump finished code for the whole project. For each step: explain the concept briefly, show me what to do, let me write or at least run the important parts myself, then review the result. When you do write code, explain the non-obvious lines and why.
- Stay on one step at a time. At the end of each step, tell me what "done" looks like (a test I can run), and wait for my confirmation before moving on.
- Ask me questions when something is unclear or depends on my setup instead of guessing, for example the exact board, the flash size, the pins I wired, my Qt kit (32 or 64 bit), or error messages.
- Do not trust the roadmap blindly. It was written from general knowledge. Verify pin numbers, the PlatformIO board name, the SDK API names, and USB descriptor details against the official documentation (RP2040 datasheet, Pico SDK docs, TinyUSB, Microsoft WinUSB and MS OS 2.0 docs) and tell me when something differs.
- Never invent register names, API functions or descriptor fields. If you are not sure, say so and check the documentation.
- Keep the DLL API clean and hardware-independent (C interface, return codes, no exceptions or STL types across the DLL boundary). I want to reuse the same DLL later with an STM32 device.
- Point out common mistakes before I make them: 32/64-bit mismatch between DLL and Qt kit, descriptors cached by Windows, blocking the Qt GUI thread, buffer overruns, signed/unsigned and endianness in the packet.
- When something fails, help me debug it systematically: what to measure, what to print, what the logic analyzer or Device Manager should show. Do not just try random fixes.
- Keep the code simple and readable. Prefer plain C or simple C++ in firmware, no clever templates. Comment the code where it teaches something.
- Keep project files organised, for example `firmware/` (PlatformIO), `dll/` (Visual Studio 2019 solution), `qt_app/`, and `docs/` for notes and the protocol description.
- At the end of each session, help me write short notes in `docs/` (what I learned, protocol, open questions).

## Start

1. Read `docs/roadmaps/project3_rp2040_zero_thermometer.md`.
2. Summarize the plan back to me in a few lines, list any assumptions you are not sure about, and ask me the questions you need answered before Session 1 (board name in PlatformIO, how I will flash, which pins I wired).
3. Then begin Session 1 and guide me step by step.
