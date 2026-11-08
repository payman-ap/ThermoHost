# 20. From this project toward device-driver development

## Begin with the function-driver boundary

The current application uses WinUSB as the device's Windows function driver.
Your DLL supplies a thermometer API above it. If a future project requires a
custom Windows USB client driver, it would usually still submit requests
through Microsoft's USB stack rather than programming the host controller
directly. The official
[USB driver-development overview](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-driver-development-guide)
explains that layering and introduces WDF and WDM.

Therefore the next step is not “replace a DLL with register writes.” It is to
learn the operating-system I/O model, request lifetime, plug-and-play, power
transitions, cancellation, concurrency, and device access policy. Your existing
protocol and tests give you a known device against which to study those ideas.

## What you already understand

The project has given you endpoints, control transfers, bulk data, descriptors,
device discovery, resource handles, buffering, and compatibility checks. These
concepts remain relevant in a framework driver. The API names and execution
contexts change, but an old response still needs correlation and a removed
device still invalidates assumptions.

The opaque handle is particularly good preparation. A driver also maintains
per-device state with defined ownership. A request can outlive the function
that initiated it. Cleanup must not free memory while another execution path
still uses it. Those are familiar concerns at a more demanding boundary.

## WDF changes the programming model

Windows Driver Frameworks provides object models and event callbacks for driver
development. KMDF is kernel-mode; UMDF is user-mode framework driver
development. Neither is simply a normal desktop DLL loaded by this console.
Microsoft's [WDF development overview](https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/using-the-framework-to-develop-a-driver)
introduces driver/device objects and callback-based execution.

In a framework project, you learn how device arrival creates driver state,
how application requests reach queues, how requests are completed, and how
device removal changes which work can still run. You must also understand the
execution-context rules of the chosen model. Copying desktop C++ calls into a
kernel callback is not a valid migration strategy.

This booklet stops at a conceptual bridge. It does not provide an installable
driver package, prescribe current signing steps, or modify this computer's
driver bindings. Those topics require their own verified environment and
current official toolchain guidance.

## Requests have lifetimes, not just return values

A synchronous desktop function makes lifetime appear simple: call, wait,
return. Internally, I/O is a request that can be pending, completed, canceled,
or affected by removal. Asynchronous APIs make that lifetime visible to the
application too.

Suppose an application submits a read and then closes. Who owns its buffer?
Who cancels pending work? Can the device complete after cancellation was
requested? Which path releases the object? These questions must have one
consistent answer. Double completion, double freeing, and use-after-free are
ownership bugs regardless of whether the code is in a DLL or driver.

Your current console avoids much of this complexity by using one synchronous
read at a time and letting main perform cleanup. That is a deliberate starting
point, not a complete model for a concurrent driver.

## Plug-and-play and power are behavior

The present application ends a session on transport errors. A mature device
integration may need to handle unplugging, replugging, suspend, resume, and
device reset while clients still exist. It must decide whether a reappeared
board is the same logical session and whether old pending work can be retried.

Automatic retry is not always harmless. A repeated “read temperature” is
usually acceptable. A repeated “activate output” command could have a different
meaning. Design command idempotency and request identifiers before promising
transparent recovery. The token scheme teaches correlation, but does not by
itself define retry semantics for arbitrary operations.

## Keep hardware timing where it can be controlled

A custom driver may offer integration or device-management benefits, but a
quarter-second converter remains a quarter-second converter. For precise
sampling, the board should usually own sampling time and send timestamped
data. Windows can manage transport, storage, visualization, and high-level
commands under a documented latency policy.

For a later audio or high-rate DAQ project, choose sample rate, buffer duration,
sequence numbering, overflow behavior, and transfer type from requirements.
Avoid carrying this thermometer's one-request/one-conversion pattern into an
unrelated waveform application without reevaluating it.

## A staged learning plan

First improve observability while preserving the protocol: expose diagnostic
stage information and record exact host timings. Next build a chunk-based
host reader with the same parser behavior. Then study asynchronous completion
and cancellation in an isolated client. These exercises remain within user
mode and teach lifetime control directly.

After that, use an official WDF sample in a dedicated learning environment.
Map sample concepts to this project: device state to `st_ThermoDevice`, USB
target/pipes to discovered endpoints, request completion to status publication,
and cleanup callbacks to resource release. Keep the known-good WinUSB version
as a comparison rather than deleting it.

Only choose a custom driver when you can name the requirement it addresses or
when the explicit goal is learning the driver model. “More low level” is a
learning direction, not a performance measurement.

## Check your understanding

1. Would a custom USB client driver normally bypass the Microsoft USB stack?
2. Which new lifetime problems appear when requests become asynchronous?
3. Why is automatic command retry a protocol-design question?
4. Name one reason to retain the known-good WinUSB implementation during driver
   experiments.

Answers: [Chapter 20](24-answers.md#chapter-20).

<!-- NAVIGATION -->

---

[Previous: 19. Real-time thinking: latency, jitter, freshness, and queues](19-real-time-and-buffering.md) · [Contents](README.md) · [Next: 21. Practical labs: turn explanations into evidence](21-practical-labs.md)
