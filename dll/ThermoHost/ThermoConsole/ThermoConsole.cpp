#include <iostream>
#include <Windows.h>

#include "../ThermoDevice/ThermoDevice.h"

/* Keep the resolved function types synchronized with the public DLL declarations. */
using GetApiVersionFunction = decltype(&Thermo_GetApiVersion);
using GetTemperatureFunction = decltype(&Thermo_GetSimulatedTemperature);
using OpenFunction = decltype(&Thermo_Open);
using CloseFunction = decltype(&Thermo_Close);
using ReadTemperatureFunction = decltype(&Thermo_ReadTemperature);

/**************************************************************************************************/

int main()
{
	/* Load the DLL before resolving or calling any of its exports. */
	HMODULE Module = LoadLibraryW(L"ThermoDevice.dll");

	if (!Module)
	{
		const DWORD ErrorCode = GetLastError();
		std::cerr << "LoadLibrary failed: " << ErrorCode << '\n';
		return 1;
	}

	/* Resolve the version query; save Windows errors before output or cleanup. */
	auto pGetApiVersion = reinterpret_cast<GetApiVersionFunction>(
		GetProcAddress(Module, "Thermo_GetApiVersion"));

	if (!pGetApiVersion)
	{
		const DWORD ErrorCode = GetLastError();
		std::cerr << "GetProcAddress failed: " << ErrorCode << '\n';
		FreeLibrary(Module);
		return 2;
	}

	/* Resolve the original helper, which does not require a device handle. */
	auto pGetTemperature = reinterpret_cast<GetTemperatureFunction>(
		GetProcAddress(Module, "Thermo_GetSimulatedTemperature"));

	if (!pGetTemperature)
	{
		const DWORD ErrorCode = GetLastError();
		std::cerr << "Temperature lookup failed: " << ErrorCode << '\n';
		FreeLibrary(Module);
		return 3;
	}

	/* Resolve device creation before attempting to open a handle. */
	auto pOpen = reinterpret_cast<OpenFunction>(
		GetProcAddress(Module, "Thermo_Open"));

	if (!pOpen)
	{
		const DWORD ErrorCode = GetLastError();
		std::cerr << "Open lookup failed: " << ErrorCode << '\n';
		FreeLibrary(Module);
		return 4;
	}

	/* Resolve cleanup before creating a device that will need to be closed. */
	auto pClose = reinterpret_cast<CloseFunction>(
		GetProcAddress(Module, "Thermo_Close"));

	if (!pClose)
	{
		const DWORD ErrorCode = GetLastError();
		std::cerr << "Close lookup failed: " << ErrorCode << '\n';
		FreeLibrary(Module);
		return 5;
	}

	/* Resolve the read operation that uses the device's private state. */
	auto pReadTemperature = reinterpret_cast<ReadTemperatureFunction>(
		GetProcAddress(Module, "Thermo_ReadTemperature"));

	if (!pReadTemperature)
	{
		const DWORD ErrorCode = GetLastError();
		std::cerr << "Read lookup failed: " << ErrorCode << '\n';
		FreeLibrary(Module);
		return 7;
	}

	/* Report the version returned by the loaded DLL. */
	const int Version = pGetApiVersion();
	std::cout << "Explicitly loaded DLL API version: " << Version << '\n';

	/* Exercise the original output-parameter helper with valid storage. */
	double TemperatureC = -999.0;

	const int Status = pGetTemperature(&TemperatureC);

	std::cout << "Read status: " << Status << '\n';

	if (Status == ThermoStatus_Ok)
	{
		std::cout << "Temperature: " << TemperatureC << " C\n";
	}

	/* Confirm that the helper rejects a missing output pointer. */
	const int InvalidStatus = pGetTemperature(0);
	std::cout << "Null pointer status: " << InvalidStatus << '\n';

	/* Open a simulated device; the DLL writes its handle into this pointer. */
	ThermoHandle* pDevice = 0;

	const int OpenStatus = pOpen(&pDevice);
	std::cout << "Open status: " << OpenStatus << '\n';

	if (OpenStatus != ThermoStatus_Ok)
	{
		FreeLibrary(Module);
		return 6;
	}

	/* A rejected read must leave the caller's output value unchanged. */
	double HandleTemperatureC = -999.0;

	const int NullHandleStatus = pReadTemperature(0, &HandleTemperatureC);

	std::cout << "Null handle status: " << NullHandleStatus << '\n';
	std::cout << "Output after rejected read: " << HandleTemperatureC << '\n';

	/* Read while the device is still open and use the output only on success. */
	const int HandleReadStatus = pReadTemperature(pDevice, &HandleTemperatureC);

	std::cout << "Handle read status: " << HandleReadStatus << '\n';

	/* Show whether a failed read preserved the caller's original value. */
	std::cout << "Output after handle read: " << HandleTemperatureC << '\n';

	if (HandleReadStatus == ThermoStatus_Ok)
	{
		std::cout << "Handle temperature: " << HandleTemperatureC << " C\n";
	}

	/* Keep the handle valid so this test isolates the missing output pointer. */
	const int NullOutputStatus = pReadTemperature(pDevice, 0);
	std::cout << "Null output status: " << NullOutputStatus << '\n';

	/* Finish all handle reads before closing; clear the caller's stale pointer. */
	pClose(pDevice);
	pDevice = 0;
	std::cout << "Device closed\n";

	/* Confirm that Open rejects a missing output slot and Close accepts null. */
	const int InvalidOpenStatus = pOpen(0);
	std::cout << "Null open status: " << InvalidOpenStatus << '\n';

	pClose(0);
	std::cout << "Null close completed\n";

	/* Release the DLL only after all device cleanup and exported calls finish. */
	FreeLibrary(Module);
	return 0;
}
