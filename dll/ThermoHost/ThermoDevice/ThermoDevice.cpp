#define THERMODEVICE_BUILD
#include "ThermoDevice.h"

#include <new>

struct st_ThermoDevice
{
	double TemperatureC = 25.0;
};

/**************************************************************************************************/

int __cdecl Thermo_GetApiVersion(void)
{
	return 2;
}

/**************************************************************************************************/

int __cdecl Thermo_GetSimulatedTemperature(double* pTemperatureC)
{
	if (!pTemperatureC)
		return ThermoStatus_InvalidArgument;

	*pTemperatureC = 25.0;

	return ThermoStatus_Ok;
}

/**************************************************************************************************/

int __cdecl Thermo_Open(ThermoHandle** ppHandle)
{
	if (!ppHandle)
		return ThermoStatus_InvalidArgument;

	*ppHandle = 0;

	auto pDevice = new (std::nothrow) st_ThermoDevice;

	if (!pDevice)
		return ThermoStatus_OutOfMemory;

	*ppHandle = pDevice;
	return ThermoStatus_Ok;
}

/**************************************************************************************************/

void __cdecl Thermo_Close(ThermoHandle* pHandle)
{
	delete pHandle;
}
