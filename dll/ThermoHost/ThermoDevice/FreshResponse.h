#pragma once

#include "ThermoDevice.h"
#include "../../../protocol/thermo_wire.h"
#include <cstdint>
#include <cstring>

/* Keep a rolling frame window across reads and timeouts. A reply is accepted
   only when its token matches the current request. */
struct st_FreshResponse
{
	std::uint8_t Bytes[THERMO_RESPONSE_SIZE] = {};
	unsigned Count = 0;

	bool Feed(std::uint8_t Byte, const std::uint8_t* pToken,
		int& Status, double* pTemperatureC)
	{
		Bytes[Count++] = Byte;
		if (Count != sizeof(Bytes))
			return false;

		if (std::memcmp(Bytes, THERMO_RESPONSE_MAGIC, 4) != 0 ||
			std::memcmp(Bytes + 16, pToken, THERMO_TOKEN_SIZE) != 0)
		{
			std::memmove(Bytes, Bytes + 1, --Count);
			return false;
		}

		Count = 0;
		Status = ThermoStatus_InvalidPacket;
		if ((Bytes[4] & 0xFEu) || Bytes[5] || Bytes[6] || Bytes[7])
			return true;

		Status = ThermoStatus_SensorFault;
		if (Bytes[4] & 1u)
			return true;

		const std::uint32_t Bits = static_cast<std::uint32_t>(Bytes[8]) |
			(static_cast<std::uint32_t>(Bytes[9]) << 8) |
			(static_cast<std::uint32_t>(Bytes[10]) << 16) |
			(static_cast<std::uint32_t>(Bytes[11]) << 24);
		std::int64_t TemperatureX100 = Bits;
		if (Bits & 0x80000000u)
			TemperatureX100 -= 0x100000000LL;

		*pTemperatureC = static_cast<double>(TemperatureX100) / 100.0;
		Status = ThermoStatus_Ok;
		return true;
	}
};
