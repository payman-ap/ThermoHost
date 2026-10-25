/* Select export declarations when compiling the DLL implementation. */
#define THERMODEVICE_BUILD
#include "ThermoDevice.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <Windows.h>
#include <SetupAPI.h>
#include <winusb.h>

/* Link the Windows USB API used by the DLL. */
#pragma comment(lib, "Winusb.lib")
#pragma comment(lib, "Setupapi.lib")

/* Own the real USB resources; keep a simulated packet until reads use USB. */
struct st_ThermoDevice
{
	/* Windows resources start empty and are released by Thermo_Close. */
	HANDLE Device = INVALID_HANDLE_VALUE;
	WINUSB_INTERFACE_HANDLE UsbInterface = 0;

	/* Endpoint addresses will be discovered when the device is opened. */
	UCHAR BulkInPipe = 0;
	UCHAR BulkOutPipe = 0;

	std::uint8_t Packet[10] =
	{
		0x01, 0x00,
		0xF6, 0x09, 0x00, 0x00,
		0xE8, 0x03, 0x00, 0x00
	};
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
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);
	SetupDiDestroyDeviceInfoList(Devices);

	if (pDevice->Device == INVALID_HANDLE_VALUE)
		return ThermoStatus_UsbError;

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

		/* Remember the first bulk endpoint in each direction. OUT is optional. */
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

	/* Receiving temperatures requires a bulk IN endpoint. */
	return pDevice->BulkInPipe ? ThermoStatus_Ok : ThermoStatus_UnsupportedDevice;
}

/**************************************************************************************************/

static int DecodeTemperaturePacket(
	const std::uint8_t* pData,
	std::size_t DataSize,
	double* pTemperatureC)
{
	/* Validate pointers and frame length before accessing any bytes. */
	if (!pData || !pTemperatureC)
		return ThermoStatus_InvalidArgument;

	if (DataSize != 10)
		return ThermoStatus_InvalidPacket;

	/* Accept the defined message type and reject reserved flag bits. */
	if ((pData[0] != 0x01u) || ((pData[1] & 0xFEu) != 0u))
		return ThermoStatus_InvalidPacket;

	/* A sensor fault makes the temperature field unusable. */
	if ((pData[1] & 0x01u) != 0u)
		return ThermoStatus_SensorFault;

	/* Assemble the little-endian field using unsigned shifts. */
	const std::uint32_t Bits =
		static_cast<std::uint32_t>(pData[2]) |
		(static_cast<std::uint32_t>(pData[3]) << 8) |
		(static_cast<std::uint32_t>(pData[4]) << 16) |
		(static_cast<std::uint32_t>(pData[5]) << 24);

	/* Interpret two's-complement values without an out-of-range signed cast. */
	std::int64_t TemperatureX100 = Bits;

	if ((Bits & 0x80000000u) != 0u)
		TemperatureX100 -= 0x100000000LL;

	/* Modify caller storage only after every validation succeeds. */
	*pTemperatureC = static_cast<double>(TemperatureX100) / 100.0;

	return ThermoStatus_Ok;
}

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
	/* Require a live handle and writable output storage. */
	if (!pHandle || !pTemperatureC)
		return ThermoStatus_InvalidArgument;

	/* Decode the simulated frame using the future USB receive format. */
	return DecodeTemperaturePacket(
		pHandle->Packet,
		sizeof(pHandle->Packet),
		pTemperatureC);
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
