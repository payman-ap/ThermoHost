#include <iostream>
#include <Windows.h>

#include "../ThermoDevice/ThermoDevice.h"

using GetApiVersionFunction = decltype(&Thermo_GetApiVersion);
using GetTemperatureFunction = decltype(&Thermo_GetSimulatedTemperature);
using OpenFunction = decltype(&Thermo_Open);
using CloseFunction = decltype(&Thermo_Close);

/**************************************************************************************************/

int main()
{
	HMODULE Module = LoadLibraryW(L"ThermoDevice.dll");

	if (!Module)
	{
		const DWORD ErrorCode = GetLastError();
		std::cerr << "LoadLibrary failed: " << ErrorCode << '\n';
		return 1;
	}

	auto pGetApiVersion = reinterpret_cast<GetApiVersionFunction>(
		GetProcAddress(Module, "Thermo_GetApiVersion"));

	if (!pGetApiVersion)
	{
		const DWORD ErrorCode = GetLastError();
		std::cerr << "GetProcAddress failed: " << ErrorCode << '\n';
		FreeLibrary(Module);
		return 2;
	}

	auto pGetTemperature = reinterpret_cast<GetTemperatureFunction>(
		GetProcAddress(Module, "Thermo_GetSimulatedTemperature"));

	if (!pGetTemperature)
	{
		const DWORD ErrorCode = GetLastError();
		std::cerr << "Temperature lookup failed: " << ErrorCode << '\n';
		FreeLibrary(Module);
		return 3;
	}

	auto pOpen = reinterpret_cast<OpenFunction>(
		GetProcAddress(Module, "Thermo_Open"));

	if (!pOpen)
	{
		const DWORD ErrorCode = GetLastError();
		std::cerr << "Open lookup failed: " << ErrorCode << '\n';
		FreeLibrary(Module);
		return 4;
	}

	auto pClose = reinterpret_cast<CloseFunction>(
		GetProcAddress(Module, "Thermo_Close"));

	if (!pClose)
	{
		const DWORD ErrorCode = GetLastError();
		std::cerr << "Close lookup failed: " << ErrorCode << '\n';
		FreeLibrary(Module);
		return 5;
	}

	const int Version = pGetApiVersion();
	std::cout << "Explicitly loaded DLL API version: " << Version << '\n';

	double TemperatureC = -999.0;

	const int Status = pGetTemperature(&TemperatureC);

	std::cout << "Read status: " << Status << '\n';

	if (Status == ThermoStatus_Ok)
	{
		std::cout << "Temperature: " << TemperatureC << " C\n";
	}

	const int InvalidStatus = pGetTemperature(0);
	std::cout << "Null pointer status: " << InvalidStatus << '\n';

	ThermoHandle* pDevice = 0;

	const int OpenStatus = pOpen(&pDevice);
	std::cout << "Open status: " << OpenStatus << '\n';

	if (OpenStatus != ThermoStatus_Ok)
	{
		FreeLibrary(Module);
		return 6;
	}

	pClose(pDevice);
	pDevice = 0;
	std::cout << "Device closed\n";

	const int InvalidOpenStatus = pOpen(0);
	std::cout << "Null open status: " << InvalidOpenStatus << '\n';

	pClose(0);
	std::cout << "Null close completed\n";

	FreeLibrary(Module);
	return 0;
}
