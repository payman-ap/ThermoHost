#include <iostream>
#include <Windows.h>
#include <SetupAPI.h>
#include <memory>
#include <new>
#include <winusb.h>

#pragma comment(lib, "Winusb.lib")

#include "../ThermoDevice/ThermoDevice.h"

/* Link the Windows device-discovery library. */
#pragma comment(lib, "Setupapi.lib")

/* Keep the resolved function types synchronized with the public DLL declarations. */
using GetApiVersionFunction = decltype(&Thermo_GetApiVersion);
using GetTemperatureFunction = decltype(&Thermo_GetSimulatedTemperature);
using OpenFunction = decltype(&Thermo_Open);
using CloseFunction = decltype(&Thermo_Close);
using ReadTemperatureFunction = decltype(&Thermo_ReadTemperature);

/**************************************************************************************************/

static void PrintThermoDevicePath(
	HDEVINFO Devices,
	SP_DEVICE_INTERFACE_DATA* pInterface)
{
	/* First ask Windows how many bytes the variable-length result needs. */
	DWORD RequiredSize = 0;

	const BOOL SizeResult = SetupDiGetDeviceInterfaceDetailW(
		Devices, pInterface, 0, 0, &RequiredSize, 0);

	const DWORD SizeError = SizeResult ? ERROR_SUCCESS : GetLastError();

	if ((SizeError != ERROR_INSUFFICIENT_BUFFER) ||
		(RequiredSize < sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W)))
	{
		std::cerr << "Path size query failed: " << SizeError << '\n';
		return;
	}

	/* Keep the buffer alive until we have finished using its device path. */
	std::unique_ptr<BYTE[]> pBuffer(new (std::nothrow) BYTE[RequiredSize]);

	if (!pBuffer)
	{
		std::cerr << "Unable to allocate device path buffer\n";
		return;
	}

	auto pDetail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(
		pBuffer.get());

	/* cbSize describes the fixed structure, not the allocated buffer size. */
	pDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

	/* Retrieve the path using the size reported by Windows. */
	if (!SetupDiGetDeviceInterfaceDetailW(
		Devices, pInterface, pDetail, RequiredSize, 0, 0))
	{
		const DWORD ErrorCode = GetLastError();
		std::cerr << "Device path query failed: " << ErrorCode << '\n';
		return;
	}

	/* The W API returns a wide-character path. */
	std::wcout << L"Device path: " << pDetail->DevicePath << L'\n';

	/* Open the device with the overlapped-I/O flag required by WinUSB. */
	HANDLE Device = CreateFileW(
		pDetail->DevicePath,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		0,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
		0);

	if (Device == INVALID_HANDLE_VALUE)
	{
		const DWORD ErrorCode = GetLastError();
		std::cerr << "CreateFile failed: " << ErrorCode << '\n';
		return;
	}

	/* Obtain the WinUSB interface handle used by subsequent USB operations. */
	WINUSB_INTERFACE_HANDLE UsbInterface = 0;

	if (!WinUsb_Initialize(Device, &UsbInterface))
	{
		const DWORD ErrorCode = GetLastError();
		std::cerr << "WinUSB initialization failed: " << ErrorCode << '\n';
		CloseHandle(Device);
		return;
	}

	std::cout << "WinUSB interface initialized\n";

	/* Query alternate setting zero of the opened interface. */
	USB_INTERFACE_DESCRIPTOR InterfaceDescriptor = {};

	if (!WinUsb_QueryInterfaceSettings(UsbInterface, 0, &InterfaceDescriptor))
	{
		const DWORD ErrorCode = GetLastError();
		std::cerr << "Interface query failed: " << ErrorCode << '\n';
		WinUsb_Free(UsbInterface);
		CloseHandle(Device);
		return;
	}

	std::cout << "Interface: "
		<< static_cast<unsigned>(InterfaceDescriptor.bInterfaceNumber)
		<< ", endpoints: "
		<< static_cast<unsigned>(InterfaceDescriptor.bNumEndpoints) << '\n';

	/* Enumerate endpoint indexes and inspect their actual addresses. */
	for (UCHAR PipeIndex = 0;
		PipeIndex < InterfaceDescriptor.bNumEndpoints;
		++PipeIndex)
	{
		WINUSB_PIPE_INFORMATION Pipe = {};

		if (!WinUsb_QueryPipe(UsbInterface, 0, PipeIndex, &Pipe))
		{
			const DWORD ErrorCode = GetLastError();
			std::cerr << "Pipe query failed: " << ErrorCode << '\n';
			WinUsb_Free(UsbInterface);
			CloseHandle(Device);
			return;
		}

		/* Endpoint direction is relative to the host: IN means board to PC. */
		std::cout << "Pipe 0x"
			<< std::hex << static_cast<unsigned>(Pipe.PipeId) << std::dec
			<< ((Pipe.PipeId & 0x80) ? " IN" : " OUT")
			<< " | " << ((Pipe.PipeType == UsbdPipeTypeBulk) ? "bulk" : "other")
			<< " | max packet " << Pipe.MaximumPacketSize << '\n';
	}

	/* Release WinUSB resources before closing the underlying device handle. */
	WinUsb_Free(UsbInterface);
	CloseHandle(Device);

	std::cout << "USB probe handles released\n";
}

/**************************************************************************************************/

static void CheckThermoDevicePresence()
{
	/* Must match DeviceInterfaceGUIDs in the board's Microsoft descriptor. */
	const GUID InterfaceGuid =
	{
		0xD026972A, 0x922E, 0x455F,
		{0xA0, 0x35, 0xB0, 0x8D, 0x3E, 0x38, 0x3F, 0xE4}
	};

	/* Obtain the set of currently present interfaces with our GUID. */
	HDEVINFO Devices = SetupDiGetClassDevsW(
		&InterfaceGuid,
		0,
		0,
		DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	if (Devices == INVALID_HANDLE_VALUE)
	{
		const DWORD ErrorCode = GetLastError();
		std::cerr << "Device discovery failed: " << ErrorCode << '\n';
		return;
	}

	/* Windows requires cbSize before filling this structure. */
	SP_DEVICE_INTERFACE_DATA Interface = {};
	Interface.cbSize = sizeof(Interface);

	const BOOL Found = SetupDiEnumDeviceInterfaces(
		Devices, 0, &InterfaceGuid, 0, &Interface);

	/* Preserve the result before releasing the information set. */
	const DWORD ErrorCode = Found ? ERROR_SUCCESS : GetLastError();

	/* No matching interface is a normal disconnected-device result. */
	if (Found)
	{
		std::cout << "Board interface found\n";
		PrintThermoDevicePath(Devices, &Interface);
	}
	else if (ErrorCode == ERROR_NO_MORE_ITEMS)
	{
		std::cout << "Board not connected\n";
	}
	else
	{
		std::cerr << "Interface enumeration failed: " << ErrorCode << '\n';
	}

	/* Release the information set after all interface queries finish. */
	SetupDiDestroyDeviceInfoList(Devices);
}

/**************************************************************************************************/

int main()
{
	CheckThermoDevicePresence();

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

	/* Open the real USB device through the DLL's opaque handle API. */
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

	/* Check that repeated reads preserve frame alignment and device state. */
	for (int Sample = 0; Sample < 20; ++Sample)
	{
		double Value = -999.0;
		const ULONGLONG ReadStarted = GetTickCount64();
		const int ReadStatus = pReadTemperature(pDevice, &Value);
		const ULONGLONG ElapsedMs = GetTickCount64() - ReadStarted;

		std::cout << "USB sample " << Sample + 1
			<< " | status " << ReadStatus;

		if (ReadStatus == ThermoStatus_Ok)
		{
			std::cout << " | " << Value << " C";
		}
		else
		{
			std::cout << " | output " << Value;
		}

		std::cout << " | elapsed " << ElapsedMs << " ms";

		std::cout << '\n';

		if (ReadStatus != ThermoStatus_Ok)
			break;
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
