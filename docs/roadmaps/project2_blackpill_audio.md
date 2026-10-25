# Project 2: BlackPill + INMP441 Mic In and I2S Audio Out (WinUSB, DLL, Qt, DSP)

Goal: capture audio from an INMP441 I2S microphone on a WeAct BlackPill (STM32F411CEU6), stream it to Windows over USB through your own DLL, analyze it in Qt (waveform, level, FFT), then add playback through an I2S DAC (PCM5102 or MAX98357). Later, optionally, turn it into a real USB sound card.

Prerequisite: Project 1 finished (WinUSB device, DLL structure, Qt app). Reuse as much as possible.

Time budget: about one day per week.

## Hardware and tools

- WeAct BlackPill (STM32F411CEU6, Cortex-M4F, 100 MHz, 128 KB RAM, USB OTG full-speed)
- INMP441 microphone (you have 4)
- Output: GY-PCM5102 I2S DAC (line out) and/or MAX98357 class-D amp (speaker, mono)
- Debug: ST-LINK V2 or WeAct mini debugger (the BlackPill also has a USB DFU bootloader via BOOT0)
- 24 MHz logic analyzer: enough for I2S at 48 kHz (BCLK about 3.07 MHz)
- Optional: Si5351 clock module if a DAC needs an MCLK
- Windows side: same as Project 1 (Visual Studio 2019, Qt 5.15, Windows SDK for WinUSB)

## Hardware fit check

- The F411 has several I2S-capable SPI peripherals (SPI2 and SPI3 are full I2S, with I2S clock from PLLI2S). One can receive the mic while the other drives the DAC, so mic plus playback fits.
- Verify exact I2S pins on the WeAct BlackPill pinout. Typical options: I2S2 on PB12 (WS), PB13 (CK), PB15 (SD); I2S3 on PA15 (WS), PB3 (CK), PB5 (SD). PA15 and PB3 are also JTAG/SWO pins, which is fine when you debug over SWD.
- USB OTG FS is full-speed (12 Mbit/s). 48 kHz x 24-bit mono is about 1.15 Mbit/s and stereo 2.3 Mbit/s, so there is plenty of headroom even for in and out together.
- If pins or timing get tight, the WeAct F446 board (MOD-019) is the fallback.

## INMP441 facts

- 24-bit output in Philips I2S format, sent in a 32-bit slot, MSB first, with the data in the upper bits. The lower bits are zero.
- No MCLK needed. The sample rate is BCLK / 64 (for 32-bit slots, stereo frame). Valid range about 7.8-50 kHz. 48 kHz gives BCLK = 3.072 MHz.
- L/R pin to GND selects the left slot, to 3.3 V the right slot. The microphone is silent in the other slot. For a mono mic, one channel is just zeros.
- It has a DC offset and a high-pass behaviour at low frequencies, so remove DC in software.
- Supply: 3.3 V. Keep wires short, since I2S clock lines are sensitive.

## Phase A: I2S capture (sessions 1-2)

Session 1: Clock and I2S bring-up
- New PlatformIO project (board `blackpill_f411ce`, framework `stm32cube`).
- Set up the clocks: 96 MHz system clock via PLL (or 100 MHz, but 96 MHz gives an exact 48 MHz USB clock).
- Configure PLLI2S so that the I2S sample rate is as close to 48 kHz as possible. Read the I2S clock table in the reference manual RM0383 and calculate the real sample rate and error.
- Configure I2S2 as master receive, Philips standard, 24-bit data in 32-bit frame, 48 kHz.
- Capture with the logic analyzer to confirm WS, CK and data.

Session 2: DMA double buffering
- With 24-bit data in a 32-bit slot the HAL transfers 16-bit half-words, so each sample arrives as two half-words. Handle that and rebuild the 24-bit value (sign-extend to int32).
- Use DMA in circular mode with half-transfer and transfer-complete callbacks (double buffering). Process one half while the DMA fills the other.
- Memory practice: place buffers in static RAM, check alignment, check the linker map. Calculate RAM use. Count missed callbacks to detect overruns.
- Print RMS level and peak over UART or the debugger, and test by clapping and talking.

Done when: you see stable audio levels, no overruns for 10 minutes, and silence reads near zero after DC removal.

## Phase B: Signal processing on the MCU (session 3)

- DC blocker: `y[n] = x[n] - x[n-1] + R*y[n-1]` with R about 0.995 (check the cutoff).
- Level metering: RMS and peak in dBFS.
- Convert 24-bit to 16-bit with correct rounding for transfer if you want to halve the data rate.
- Optional: simple IIR/FIR filter using the Cortex-M4F FPU or CMSIS-DSP.
- Think about fixed-point versus float and about the cost per sample at 48 kHz.

Done when: level meter works and DC offset is gone.

## Phase C: USB streaming to the DLL (sessions 4-5)

- Reuse the vendor-specific WinUSB device from Project 1. Add a bulk IN endpoint for audio, using packets such as 512 or 1024 bytes of samples plus a small header (sequence number, sample rate, format, flags).
- The sequence number lets the PC detect lost packets.
- Firmware: a ring buffer between the DMA callbacks and USB transmit. Make the buffer size a compile-time constant, and handle overflow by counting and flagging it, never by blocking the DMA.
- Extend the DLL with a streaming API, for example:

```c
MYDEV_API int MyAudio_Start(MyDevHandle* h, unsigned sampleRate);
MYDEV_API int MyAudio_Read(MyDevHandle* h, int32_t* samples, unsigned maxSamples, unsigned timeout_ms);
MYDEV_API int MyAudio_Stop(MyDevHandle* h);
MYDEV_API int MyAudio_GetStats(MyDevHandle* h, MyAudioStats* stats); // dropped packets, overruns
```

- In the DLL, use overlapped `WinUsb_ReadPipe` with several transfers queued at once, so the USB pipe never runs dry. A worker thread fills a thread-safe ring buffer, and `MyAudio_Read` drains it. Practice locking and lock-free queues here.
- Note that bulk transfers have no timing guarantee, but they are fine for this learning stage.

Done when: 10 minutes of streaming with zero lost packets and a correct waveform on the PC.

## Phase D: Qt analysis app (session 6)

- Show the waveform (scrolling) and a level meter.
- FFT spectrum: use a window (Hann), a library such as KissFFT or FFTW, and show dBFS. Check a test tone (play a 1 kHz tone from your phone) shows the peak at the right frequency.
- Save to WAV (write the header yourself, 24-bit or 16-bit, mono, 48 kHz) and verify it opens in Audacity.
- Keep the DLL reading in a `QThread` and pass data to the GUI with signals, queued connections.
- Optional: use the logic analyzer to measure USB latency indirectly via a GPIO toggle.

## Phase E: Playback path (sessions 7-8)

- Add I2S3 as master transmit to the PCM5102 (check its solder jumpers: with no MCLK, SCK is tied to GND so its PLL generates the clock) or to the MAX98357 (no MCLK needed, gain/channel set by the SD pin).
- Start with a sine wave generated in firmware to prove the output path.
- Then add a bulk OUT endpoint: the PC sends audio, the firmware puts it in a ring buffer and the DMA plays it.
- Clock problem: the PC clock and the MCU I2S clock are not synchronized, so the playback buffer slowly fills or drains. First handle it simply (drop or repeat a sample when the buffer gets too full or empty, and count those events). Later learn how real USB audio solves it with isochronous transfers and a feedback endpoint.
- Optional loopback test: mic in, DSP, through the PC, back out to the DAC.

Done when: audio from a WAV file or a tone generator in Qt plays cleanly without clicks for several minutes.

## Phase F: Stretch goals

- USB Audio Class firmware (UAC1 or UAC2) with isochronous endpoints. Windows then provides the sound card automatically with its built-in class driver, so it shows in the Sound settings and works with any program. You compare it with your own DLL approach and with your USB sound card (MOD-023): inspect its descriptors with a tool such as USBTreeView.
- ASIO-like low latency in your DLL.
- A real kernel/audio driver is a separate, much bigger project, and would need test-signing or a second machine.

## Skills practiced

- Embedded: PLL/I2S clocks, DMA and double buffering, ring buffers, interrupts, linker map, RAM budgeting, fixed versus floating point
- Signal processing: DC removal, dBFS, windowing, FFT, filtering, sample rate and clock accuracy
- Windows: overlapped I/O, threads, thread-safe queues, WinUSB streaming
- Qt: threads, plotting, WAV export

## Common problems

- Noise or wrong values: bit alignment (24-bit in 32-bit slot), wrong channel (L/R pin), or half-word order from DMA.
- Clicks: ring buffer under/overrun, or the PC thread starved. Count and log events.
- Wrong pitch: sample rate is not what you think, so calculate the actual I2S clock from the PLL settings.
- Loses packets: queue more overlapped reads, and increase buffer sizes.
- The INMP441 outputs nothing: check 3.3 V supply, the L/R pin, and that the WS/CK signals are present.
