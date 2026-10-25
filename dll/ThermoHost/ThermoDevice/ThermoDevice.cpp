#define THERMODEVICE_BUILD
#include "ThermoDevice.h"

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
