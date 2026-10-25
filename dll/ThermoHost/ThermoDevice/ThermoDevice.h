#pragma once

/* Share declarations between the exporting DLL and importing clients. */
#ifdef THERMODEVICE_BUILD
#define THERMODEVICE_API __declspec(dllexport)
#else
#define THERMODEVICE_API __declspec(dllimport)
#endif

/* Return operation status separately from measured values. */
enum ThermoStatus
{
	ThermoStatus_Ok = 0,
	ThermoStatus_InvalidArgument = 1,
	ThermoStatus_OutOfMemory = 2,
	ThermoStatus_InvalidPacket = 3,
	ThermoStatus_SensorFault = 4,
	ThermoStatus_DeviceNotFound = 5,
	ThermoStatus_UsbError = 6,
	ThermoStatus_UnsupportedDevice = 7
};

/* Expose the handle type without revealing the device layout. */
typedef struct st_ThermoDevice ThermoHandle;

/* Use C linkage for exports while allowing this header in C or C++ code. */
#ifdef __cplusplus
extern "C" {
#endif

	THERMODEVICE_API int __cdecl Thermo_GetApiVersion(void);

	/* Writes degrees Celsius on success; leaves output unchanged on failure.
	   The caller must provide a pointer to a writable double. */
	THERMODEVICE_API int __cdecl Thermo_GetSimulatedTemperature(double* pTemperatureC);

	/* Opens the first present compatible USB device. Pass an empty output slot.
	   On failure, a valid output slot is set to null. */
	THERMODEVICE_API int __cdecl Thermo_Open(ThermoHandle** ppHandle);

	/* Requires a live handle returned by Thermo_Open.
	   Writes degrees Celsius on success; leaves output unchanged on failure.
	   Null arguments are rejected. */
	THERMODEVICE_API int __cdecl Thermo_ReadTemperature(
		ThermoHandle* pHandle,
		double* pTemperatureC);

	/* Closes a handle returned by Thermo_Open. Null is allowed.
	   Close each live handle exactly once, before unloading the DLL. */
	THERMODEVICE_API void __cdecl Thermo_Close(ThermoHandle* pHandle);

#ifdef __cplusplus
}
#endif
