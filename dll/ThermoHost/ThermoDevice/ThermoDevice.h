#pragma once

#ifdef THERMODEVICE_BUILD
#define THERMODEVICE_API __declspec(dllexport)
#else
#define THERMODEVICE_API __declspec(dllimport)
#endif

enum ThermoStatus
{
	ThermoStatus_Ok = 0,
	ThermoStatus_InvalidArgument = 1,
	ThermoStatus_OutOfMemory = 2
};

typedef struct st_ThermoDevice ThermoHandle;

#ifdef __cplusplus
extern "C" {
#endif

	THERMODEVICE_API int __cdecl Thermo_GetApiVersion(void);

	/* Writes degrees Celsius on success; leaves output unchanged on failure.
	   The caller must provide a pointer to a writable double. */
	THERMODEVICE_API int __cdecl Thermo_GetSimulatedTemperature(double* pTemperatureC);

	/* Opens a simulated device. Pass an empty output slot.
	   On failure, a valid output slot is set to null. */
	THERMODEVICE_API int __cdecl Thermo_Open(ThermoHandle** ppHandle);

	/* Closes a handle returned by Thermo_Open. Null is allowed.
	   Close each live handle exactly once, before unloading the DLL. */
	THERMODEVICE_API void __cdecl Thermo_Close(ThermoHandle* pHandle);

#ifdef __cplusplus
}
#endif
