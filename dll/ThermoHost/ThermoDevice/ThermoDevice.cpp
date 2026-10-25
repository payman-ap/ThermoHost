/* Select export declarations when compiling the DLL implementation. */
#define THERMODEVICE_BUILD
#include "ThermoDevice.h"
#include "FreshResponse.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <Windows.h>
#include <SetupAPI.h>
#include <winusb.h>
#include <bcrypt.h>

/* Link the Windows USB API used by the DLL. */
#pragma comment(lib, "Winusb.lib")
#pragma comment(lib, "Setupapi.lib")
#pragma comment(lib, "Bcrypt.lib")

/* Own the real USB resources and receive state for one open device. */
struct st_ThermoDevice
{
	/* Windows resources start empty and are released by Thermo_Close. */
	HANDLE Device = INVALID_HANDLE_VALUE;
	WINUSB_INTERFACE_HANDLE UsbInterface = 0;

	/* Endpoint addresses will be discovered when the device is opened. */
	UCHAR BulkInPipe = 0;
	UCHAR BulkOutPipe = 0;

	/* Assemble a complete wire frame across successful reads. */
	st_FreshResponse Response;
};

/**************************************************************************************************/

/* Acquire resources into an empty private object. On failure, the caller must
   pass that object to Thermo_Close to release any partially acquired resources. */
static int OpenUsbDevice(st_ThermoDevice* pDevice)
{
	/* Match the interface GUID registered by the firmware, not a setup-class GUID. */
	const GUID InterfaceGuid =
	{
		0xD026972A, 0x922E, 0x455F,
		{0xA0, 0x35, 0xB0, 0x8D, 0x3E, 0x38, 0x3F, 0xE4}
	};

	/* Search only interfaces that are currently present. */
	HDEVINFO Devices = SetupDiGetClassDevsW(
		&InterfaceGuid, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	if (Devices == INVALID_HANDLE_VALUE)
		return ThermoStatus_UsbError;

	SP_DEVICE_INTERFACE_DATA Interface = {};
	Interface.cbSize = sizeof(Interface);

	if (!SetupDiEnumDeviceInterfaces(Devices, 0, &InterfaceGuid, 0, &Interface))
	{
		const DWORD ErrorCode = GetLastError();
		SetupDiDestroyDeviceInfoList(Devices);
		return (ErrorCode == ERROR_NO_MORE_ITEMS)
			? ThermoStatus_DeviceNotFound : ThermoStatus_UsbError;
	}

	/* Query and allocate the variable-sized device path, entirely inside the DLL. */
	DWORD RequiredSize = 0;
	const BOOL SizeResult = SetupDiGetDeviceInterfaceDetailW(
		Devices, &Interface, 0, 0, &RequiredSize, 0);
	const DWORD SizeError = SizeResult ? ERROR_SUCCESS : GetLastError();

	if ((SizeError != ERROR_INSUFFICIENT_BUFFER) ||
		(RequiredSize < sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W)))
	{
		SetupDiDestroyDeviceInfoList(Devices);
		return ThermoStatus_UsbError;
	}

	std::unique_ptr<BYTE[]> pBuffer(new (std::nothrow) BYTE[RequiredSize]);
	if (!pBuffer)
	{
		SetupDiDestroyDeviceInfoList(Devices);
		return ThermoStatus_OutOfMemory;
	}

	auto pDetail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(pBuffer.get());
	pDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

	if (!SetupDiGetDeviceInterfaceDetailW(
		Devices, &Interface, pDetail, RequiredSize, 0, 0))
	{
		SetupDiDestroyDeviceInfoList(Devices);
		return ThermoStatus_UsbError;
	}

	/* Retain the open device handle; the path buffer is no longer needed afterward. */
	pDevice->Device = CreateFileW(
		pDetail->DevicePath,
		GENERIC_READ | GENERIC_WRITE,
		0,
		0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);
	SetupDiDestroyDeviceInfoList(Devices);

	if (pDevice->Device == INVALID_HANDLE_VALUE)
		return GetLastError() == ERROR_SHARING_VIOLATION
			? ThermoStatus_DeviceBusy : ThermoStatus_UsbError;

	if (!WinUsb_Initialize(pDevice->Device, &pDevice->UsbInterface))
		return ThermoStatus_UsbError;

	/* Verify the vendor interface before accepting its bulk endpoints. */
	USB_INTERFACE_DESCRIPTOR InterfaceDescriptor = {};
	if (!WinUsb_QueryInterfaceSettings(pDevice->UsbInterface, 0, &InterfaceDescriptor))
		return ThermoStatus_UsbError;

	if (InterfaceDescriptor.bInterfaceClass != 0xFF)
		return ThermoStatus_UnsupportedDevice;

	for (UCHAR PipeIndex = 0; PipeIndex < InterfaceDescriptor.bNumEndpoints; ++PipeIndex)
	{
		WINUSB_PIPE_INFORMATION Pipe = {};
		if (!WinUsb_QueryPipe(pDevice->UsbInterface, 0, PipeIndex, &Pipe))
			return ThermoStatus_UsbError;

		if (Pipe.PipeType != UsbdPipeTypeBulk)
			continue;

		/* Requests use OUT; matching responses use IN. */
		if ((Pipe.PipeId & 0x80) != 0)
		{
			if (!pDevice->BulkInPipe)
				pDevice->BulkInPipe = Pipe.PipeId;
		}
		else if (!pDevice->BulkOutPipe)
		{
			pDevice->BulkOutPipe = Pipe.PipeId;
		}
	}

	if (!pDevice->BulkInPipe || !pDevice->BulkOutPipe)
		return ThermoStatus_UnsupportedDevice;

	/* Reject streaming firmware before any temperature can be mistaken as fresh. */
	WINUSB_SETUP_PACKET Setup = {};
	Setup.RequestType = 0xC0;
	Setup.Request = THERMO_CAPABILITY_REQUEST;
	Setup.Length = 5;
	UCHAR Capabilities[5] = {};
	const UCHAR Expected[] = {'T', 'H', 2, THERMO_REQUEST_SIZE, THERMO_RESPONSE_SIZE};
	ULONG Received = 0;
	if (!WinUsb_ControlTransfer(pDevice->UsbInterface, Setup, Capabilities,
		sizeof(Capabilities), &Received, 0) || Received != sizeof(Capabilities) ||
		std::memcmp(Capabilities, Expected, sizeof(Expected)) != 0)
		return ThermoStatus_UnsupportedDevice;

	/* Preserve unread bytes when assembling a frame one byte at a time. */
	UCHAR AllowPartial = TRUE;
	UCHAR AutoFlush = FALSE;
	if (!WinUsb_SetPipePolicy(pDevice->UsbInterface, pDevice->BulkInPipe,
		ALLOW_PARTIAL_READS, sizeof(AllowPartial), &AllowPartial) ||
		!WinUsb_SetPipePolicy(pDevice->UsbInterface, pDevice->BulkInPipe,
		AUTO_FLUSH, sizeof(AutoFlush), &AutoFlush))
		return ThermoStatus_UsbError;

	return ThermoStatus_Ok;
}

/**************************************************************************************************/

int __cdecl Thermo_GetApiVersion(void)
{
	/* Version 3 guarantees a requested conversion instead of queued samples. */
	return 3;
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
	/* Require caller-owned storage and clear it before attempting to open. */
	if (!ppHandle)
		return ThermoStatus_InvalidArgument;

	*ppHandle = 0;

	/* Allocate private state with empty Windows resource handles. */
	auto pDevice = new (std::nothrow) st_ThermoDevice;

	if (!pDevice)
		return ThermoStatus_OutOfMemory;

	/* Discover the real board, open WinUSB, and remember its bulk endpoints. */
	const int Status = OpenUsbDevice(pDevice);

	if (Status != ThermoStatus_Ok)
	{
		/* Release any resources acquired before the failure. */
		Thermo_Close(pDevice);
		return Status;
	}

	/* Publish the opaque handle only after initialization succeeds. */
	*ppHandle = pDevice;
	return ThermoStatus_Ok;
}

/**************************************************************************************************/

int __cdecl Thermo_ReadTemperature(
	ThermoHandle* pHandle,
	double* pTemperatureC)
{
	/* Validate arguments before touching USB or caller storage. */
	if (!pHandle || !pTemperatureC)
		return ThermoStatus_InvalidArgument;

	/* A random token distinguishes this read from late replies and older sessions. */
	std::uint8_t Request[THERMO_REQUEST_SIZE] = {};
	std::memcpy(Request, THERMO_REQUEST_MAGIC, 4);
	if (BCryptGenRandom(0, Request + 4, THERMO_TOKEN_SIZE,
		BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0)
		return ThermoStatus_UsbError;

	/* Bound the command and its response together, including stale-frame skipping. */
	const ULONG TimeoutMs = 1000;
	const ULONGLONG Started = GetTickCount64();
	ULONG WriteTimeout = TimeoutMs;
	if (!WinUsb_SetPipePolicy(pHandle->UsbInterface, pHandle->BulkOutPipe,
		PIPE_TRANSFER_TIMEOUT, sizeof(WriteTimeout), &WriteTimeout))
		return ThermoStatus_UsbError;

	ULONG Written = 0;
	if (!WinUsb_WritePipe(pHandle->UsbInterface, pHandle->BulkOutPipe,
		Request, sizeof(Request), &Written, 0))
		return GetLastError() == ERROR_SEM_TIMEOUT
			? ThermoStatus_Timeout : ThermoStatus_UsbError;
	if (Written != sizeof(Request))
		return ThermoStatus_UsbError;

	while (true)
	{
		const ULONGLONG Elapsed = GetTickCount64() - Started;
		if (Elapsed >= TimeoutMs)
			return ThermoStatus_Timeout;

		ULONG Remaining = TimeoutMs - static_cast<ULONG>(Elapsed);
		if (!WinUsb_SetPipePolicy(pHandle->UsbInterface, pHandle->BulkInPipe,
			PIPE_TRANSFER_TIMEOUT, sizeof(Remaining), &Remaining))
			return ThermoStatus_UsbError;

		/* Read one byte into the persistent frame window. WinUSB retains the rest
		   of each short USB packet (ALLOW_PARTIAL_READS default is enabled). */
		UCHAR Byte = 0;
		ULONG Received = 0;
		if (!WinUsb_ReadPipe(pHandle->UsbInterface, pHandle->BulkInPipe,
			&Byte, 1, &Received, 0))
			return GetLastError() == ERROR_SEM_TIMEOUT
				? ThermoStatus_Timeout : ThermoStatus_UsbError;

		int Status = ThermoStatus_InvalidPacket;
		if (Received && pHandle->Response.Feed(Byte, Request + 4, Status, pTemperatureC))
			return Status;
	}
}

/**************************************************************************************************/

void __cdecl Thermo_Close(ThermoHandle* pHandle)
{
	/* Closing an empty handle remains a harmless operation. */
	if (!pHandle)
		return;

	/* Release the WinUSB interface before its underlying Windows handle. */
	if (pHandle->UsbInterface)
	{
		WinUsb_Free(pHandle->UsbInterface);
	}

	if (pHandle->Device != INVALID_HANDLE_VALUE)
	{
		CloseHandle(pHandle->Device);
	}

	/* The DLL also owns the private device object itself. */
	delete pHandle;
}
