# Appendix C. Further reading

The chapters introduce the concepts before citing implementation details.
Use these primary sources to resolve exact API behavior and version-dependent
requirements. Generic definitions may have useful encyclopedia explanations,
but controller, stack, and operating-system contracts belong in their own
technical documentation.

## USB architecture and descriptors

- [USB-IF specification library entry](https://www.usb.org/document-library/usb-20-specification): starting point for the USB 2.0 base specification.
- [Infineon USB 101](https://www.infineon.com/dgdl/Infineon-AN57294_USB_101_An_Introduction_to_Universal_Serial_Bus_2.0-ApplicationNotes-v09_00-EN.pdf?fileId=8ac78c8c7cdc391c017d072d8e8e5256): vendor introduction to signaling, packets, descriptors, and enumeration.
- [Microsoft endpoints and pipes](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-endpoints-and-their-pipes), [device descriptors](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-device-descriptors), and [configuration descriptors](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-configuration-descriptors): host-side terminology and descriptor hierarchy.
- [USB 3.2 naming](https://www.usb.org/sites/default/files/usb_3_2_language_product_and_packaging_guidelines_final.pdf), [USB4 system overview](https://www.usb.org/sites/default/files/D1T1-3%20-%20USB4%20System%20Overview.pdf), and [USB4 version 2.0 announcement](https://www.usb.org/sites/default/files/2022-10/USB-IF%20USB%2080Gbps%20Announcement_FINAL_v2.pdf): rate families and architecture, distinct from payload performance.

## Operating-system access

- [WinUSB architecture](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/winusb-architecture) and [driver-model comparison](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/winusb-considerations): Microsoft components and model selection.
- [WinUSB device binding](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/automatic-installation-of-winusb) and [OS descriptors](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors): firmware-assisted Windows metadata.
- [WinUSB API walkthrough](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/using-winusb-api-to-communicate-with-a-usb-device): discovery, initialization, endpoint queries, transfers.
- [WinUSB initialization](https://learn.microsoft.com/en-us/windows/win32/api/winusb/nf-winusb-winusb_initialize), [reads](https://learn.microsoft.com/en-us/windows/win32/api/winusb/nf-winusb-winusb_readpipe), and [pipe policies](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/winusb-functions-for-pipe-policy-modification): exact handle and buffering behavior.
- [libusb overview](https://libusb.info/), [device handling](https://libusb.sourceforge.io/api-1.0/group__libusb__dev.html), [synchronous I/O](https://libusb.sourceforge.io/api-1.0/group__libusb__syncio.html), and [asynchronous I/O](https://libusb.sourceforge.io/api-1.0/group__libusb__asyncio.html): portable user-space interfaces.
- [Linux host-side USB API](https://www.kernel.org/doc/html/latest/driver-api/usb/usb.html): kernel-side drivers and requests, distinct from user-space APIs.

## Embedded implementation

- [TinyUSB architecture](https://docs.tinyusb.org/en/latest/reference/architecture.html), [integration](https://docs.tinyusb.org/en/latest/integration.html), and [device classes](https://docs.tinyusb.org/en/latest/reference/class/device.html): controller/core/class layers and application responsibilities.
- [RP2040 datasheet](https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf) and [Pico SDK hardware APIs](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html): boot, pins, SPI, USB peripheral, and SDK operations.
- [MAX6675 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/max6675.pdf): word format, conversion timing, and electrical/measurement limits.

## Capacity, streaming, and later driver study

- [Microchip USB frames](https://developerhelp.microchip.com/xwiki/bin/view/applications/usb/how-it-works/frame/): idealized packet budgets used as ceilings in the calculations.
- [USB bandwidth allocation](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-bandwidth-allocation), [isochronous endpoints](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/transfer-data-to-isochronous-endpoints), and [WinUSB isochronous operations](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/getting-set-up-to-use-windows-devices-usb): service intervals and host implementation considerations.
- [Windows USB driver development](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-driver-development-guide) and [WDF overview](https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/using-the-framework-to-develop-a-driver): framework/client-driver concepts after the user-space foundation.

## Repository evidence and reproducibility

[protocol.md](../protocol.md) specifies the current exchange;
[fresh_reads.md](../fresh_reads.md) records its validation status. Historical
[protocol v1](../protocol_v1.md) and [integration notes](../live_thermometer.md)
describe earlier learning stages, not the current decoder contract.

The book's calculated scenarios are declared models. Run
[check_calculations.py](tools/check_calculations.py) to reproduce selected
arithmetic, and [check_booklet.py](tools/check_booklet.py) to check local links,
navigation, code excerpts, and figure metadata. Neither script performs hardware
acceptance or proves all prose claims.

<!-- NAVIGATION -->

---

[Previous: Appendix B. Software provenance and source atlas](appendix-b-source-atlas.md) · [Contents](README.md)
