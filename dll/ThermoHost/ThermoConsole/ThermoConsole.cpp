#include "../ThermoDevice/ThermoDevice.h"
#include <Windows.h>
#include <atomic>
#include <charconv>
#include <cstring>
#include <iostream>

/* The handler only requests a stop; main owns DLL and device cleanup. */
static std::atomic<bool> StopRequested{false};

static BOOL WINAPI HandleConsoleSignal(DWORD Signal)
{
	if (Signal != CTRL_C_EVENT && Signal != CTRL_BREAK_EVENT)
		return FALSE;
	StopRequested.store(true);
	return TRUE;
}

static const char* StatusName(int Status)
{
	switch (Status)
	{
	case ThermoStatus_Ok: return "OK";
	case ThermoStatus_InvalidArgument: return "Invalid argument";
	case ThermoStatus_OutOfMemory: return "Out of memory";
	case ThermoStatus_InvalidPacket: return "Invalid packet";
	case ThermoStatus_SensorFault: return "Thermocouple disconnected";
	case ThermoStatus_DeviceNotFound: return "Board not connected";
	case ThermoStatus_UsbError: return "USB error";
	case ThermoStatus_UnsupportedDevice: return "Unsupported firmware: flash thermometer_usb.uf2";
	case ThermoStatus_Timeout: return "Read timed out";
	case ThermoStatus_DeviceBusy: return "Device already in use";
	default: return "Unknown status";
	}
}

/* Default is continuous. A positive sample limit is useful for repeatable tests. */
int main(int ArgumentCount, char** ppArguments)
{
	unsigned SampleLimit = 0;
	unsigned IntervalMs = 0;
	for (int Index = 1; Index < ArgumentCount; ++Index)
	{
		const char* pOption = ppArguments[Index];
		if (std::strcmp(pOption, "--help") == 0)
		{
			std::cout << "ThermoConsole [--samples N] [--interval-ms N]\n"
				"Default: fresh reads continuously until Ctrl+C.\n"
				"--samples: positive read-attempt limit; --interval-ms: extra pause (0..60000).\n";
			return 0;
		}
		const bool IsSamples = std::strcmp(pOption, "--samples") == 0;
		if ((!IsSamples && std::strcmp(pOption, "--interval-ms") != 0) ||
			++Index == ArgumentCount)
		{
			std::cerr << "Invalid option. Use --help.\n";
			return 2;
		}
		const char* pValue = ppArguments[Index];
		unsigned Value = 0;
		const auto Result = std::from_chars(pValue, pValue + std::strlen(pValue), Value);
		if (Result.ec != std::errc() || *Result.ptr ||
			(IsSamples ? Value == 0 : Value > 60000))
		{
			std::cerr << "Invalid numeric argument. Use --help.\n";
			return 2;
		}
		if (IsSamples)
			SampleLimit = Value;
		else
			IntervalMs = Value;
	}

	/* Resolve DLL exports at runtime; no direct calls or import library required. */
	HMODULE Module = LoadLibraryW(L"ThermoDevice.dll");
	if (!Module)
	{
		std::cerr << "LoadLibrary failed: " << GetLastError() << '\n';
		return 3;
	}
	auto pVersion = reinterpret_cast<decltype(&Thermo_GetApiVersion)>(
		GetProcAddress(Module, "Thermo_GetApiVersion"));
	auto pOpen = reinterpret_cast<decltype(&Thermo_Open)>(GetProcAddress(Module, "Thermo_Open"));
	auto pRead = reinterpret_cast<decltype(&Thermo_ReadTemperature)>(
		GetProcAddress(Module, "Thermo_ReadTemperature"));
	auto pClose = reinterpret_cast<decltype(&Thermo_Close)>(GetProcAddress(Module, "Thermo_Close"));
	if (!pVersion || !pOpen || !pRead || !pClose)
	{
		std::cerr << "Missing DLL export.\n";
		FreeLibrary(Module);
		return 4;
	}
	const int Version = pVersion();
	std::cout << "Explicitly loaded DLL API version: " << Version << '\n';
	if (Version != 3)
	{
		std::cerr << "Fresh reads require DLL API version 3. Rebuild the solution.\n";
		FreeLibrary(Module);
		return 4;
	}

	ThermoHandle* pDevice = 0;
	int Status = pOpen(&pDevice);
	if (Status != ThermoStatus_Ok)
	{
		std::cerr << "Open: " << StatusName(Status) << " (" << Status << ")\n";
		FreeLibrary(Module);
		return 5;
	}
	if (!SetConsoleCtrlHandler(HandleConsoleSignal, TRUE))
	{
		std::cerr << "Cannot install Ctrl+C handler.\n";
		pClose(pDevice);
		FreeLibrary(Module);
		return 6;
	}
	std::cout << "Fresh conversions; press Ctrl+C to stop.\n";

	/* Faults remain visible and recoverable; transport failures end this session. */
	unsigned Samples = 0;
	int ExitCode = 0;
	while (!StopRequested.load() && (!SampleLimit || Samples < SampleLimit))
	{
		double TemperatureC = -999.0;
		const ULONGLONG Started = GetTickCount64();
		Status = pRead(pDevice, &TemperatureC);
		std::cout << "Sample " << ++Samples << " | " << StatusName(Status)
			<< " (" << Status << ")";
		if (Status == ThermoStatus_Ok)
			std::cout << " | " << TemperatureC << " C";
		std::cout << " | elapsed " << GetTickCount64() - Started << " ms" << std::endl;
		if (Status != ThermoStatus_Ok && Status != ThermoStatus_SensorFault)
		{
			ExitCode = 7;
			break;
		}
		if (SampleLimit && Samples == SampleLimit)
			break;
		const ULONGLONG PauseStarted = GetTickCount64();
		while (!StopRequested.load() && GetTickCount64() - PauseStarted < IntervalMs)
			Sleep(10);
	}

	/* Always release device resources before unloading the code that owns them. */
	pClose(pDevice);
	SetConsoleCtrlHandler(HandleConsoleSignal, FALSE);
	FreeLibrary(Module);
	std::cout << "Device closed after " << Samples << " read attempts.\n";
	return ExitCode;
}
