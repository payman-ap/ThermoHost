/* Select export declarations when compiling the DLL implementation. */
#define THERMODEVICE_BUILD
#include "ThermoDevice.h"

#include <new>

/* The client sees only an opaque handle; the DLL owns this private state. */
struct st_ThermoDevice
{
	double TemperatureC = 25.0;
};

/**************************************************************************************************/

int __cdecl Thermo_GetApiVersion(void)
{
	/* Retain the version value used by the DLL replacement exercise. */
	return 2;
}

/**************************************************************************************************/

int __cdecl Thermo_GetSimulatedTemperature(double* pTemperatureC)
{
	/* Reject missing storage before attempting to write through the pointer. */
	if (!pTemperatureC)
		return ThermoStatus_InvalidArgument;

	/* This original helper returns a fixed value without opening a device. */
	*pTemperatureC = 25.0;

	return ThermoStatus_Ok;
}

/**************************************************************************************************/

int __cdecl Thermo_Open(ThermoHandle** ppHandle)
{
	/* The caller must provide a writable pointer slot for the new handle. */
	if (!ppHandle)
		return ThermoStatus_InvalidArgument;

	/* Leave a null output on allocation failure. */
	*ppHandle = 0;

	/* Allocate in the DLL; scalar initialization does not throw. */
	auto pDevice = new (std::nothrow) st_ThermoDevice;

	if (!pDevice)
		return ThermoStatus_OutOfMemory;

	/* Publish the fully initialized device only after allocation succeeds. */
	*ppHandle = pDevice;
	return ThermoStatus_Ok;
}

/**************************************************************************************************/

int __cdecl Thermo_ReadTemperature(
	ThermoHandle* pHandle,
	double* pTemperatureC)
{
	/* Validate both arguments before reading state or modifying caller storage. */
	if (!pHandle || !pTemperatureC)
		return ThermoStatus_InvalidArgument;

	/* The caller must supply a live handle; its fields remain private to the DLL. */
	*pTemperatureC = pHandle->TemperatureC;

	return ThermoStatus_Ok;
}

/**************************************************************************************************/

void __cdecl Thermo_Close(ThermoHandle* pHandle)
{
	/* Match the DLL's allocation; deleting null is safe. */
	delete pHandle;
}
