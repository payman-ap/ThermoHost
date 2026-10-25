/* Select export declarations when compiling the DLL implementation. */
#define THERMODEVICE_BUILD
#include "ThermoDevice.h"
#include <cstddef>
#include <cstdint>
#include <new>

/* Simulate the bytes that will eventually arrive from USB. */
struct st_ThermoDevice
{
	std::uint8_t Packet[10] =
	{
		0x01, 0x00,
		0xF6, 0x09, 0x00, 0x00,
		0xE8, 0x03, 0x00, 0x00
	};
};

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
	/* Match the DLL's allocation; deleting null is safe. */
	delete pHandle;
}
