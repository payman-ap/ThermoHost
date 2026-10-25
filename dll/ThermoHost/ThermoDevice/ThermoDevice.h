#pragma once

#ifdef THERMODEVICE_BUILD
#define THERMODEVICE_API __declspec(dllexport)
#else
#define THERMODEVICE_API __declspec(dllimport)
#endif

enum ThermoStatus
{
	ThermoStatus_Ok = 0,
	ThermoStatus_InvalidArgument = 1
};

#ifdef __cplusplus
extern "C" {
#endif

	THERMODEVICE_API int __cdecl Thermo_GetApiVersion(void);

	/* Writes degrees Celsius on success; leaves output unchanged on failure.
   The caller must provide a pointer to a writable double. */
	THERMODEVICE_API int __cdecl Thermo_GetSimulatedTemperature(double* pTemperatureC);

#ifdef __cplusplus
}
#endif
