#include "../dll/ThermoHost/ThermoDevice/FreshResponse.h"
#include <cassert>
#include <iostream>

/* Exercise the same persistent parser used by the USB read path. */
int main()
{
	std::uint8_t Token[THERMO_TOKEN_SIZE] = {42};
	std::uint8_t Frame[THERMO_RESPONSE_SIZE] = {};
	std::memcpy(Frame, THERMO_RESPONSE_MAGIC, 4);
	std::memcpy(Frame + 16, Token, sizeof(Token));
	Frame[8] = 0xFF; Frame[9] = 0xFB; Frame[10] = 0xFF; Frame[11] = 0xFF;
	st_FreshResponse Parser;
	double Temperature = -999;
	int Status = -1;

	/* A response from a timed-out request must not publish its temperature. */
	Frame[16] = 41;
	for (auto Byte : Frame)
		assert(!Parser.Feed(Byte, Token, Status, &Temperature));
	assert(Temperature == -999);

	/* Resume with a current reply after stale data, noise, and a partial prefix. */
	Parser.Feed('T', Token, Status, &Temperature);
	Parser.Feed('H', Token, Status, &Temperature);
	Frame[16] = 42;
	for (unsigned Index = 0; Index < sizeof(Frame) - 1; ++Index)
		assert(!Parser.Feed(Frame[Index], Token, Status, &Temperature));
	assert(Temperature == -999);
	assert(Parser.Feed(Frame[31], Token, Status, &Temperature));
	assert(Status == ThermoStatus_Ok && Temperature == -10.25);

	/* Fault and reserved-bit errors preserve caller storage. */
	for (unsigned Flags : {1u, 2u})
	{
		Temperature = -999;
		Frame[4] = static_cast<std::uint8_t>(Flags);
		for (auto Byte : Frame)
			Parser.Feed(Byte, Token, Status, &Temperature);
		assert(Status == (Flags == 1 ? ThermoStatus_SensorFault : ThermoStatus_InvalidPacket));
		assert(Temperature == -999);
	}

	/* A new request after a partial old reply must reject the old token. */
	Frame[4] = 0;
	for (unsigned Index = 0; Index < 17; ++Index)
		Parser.Feed(Frame[Index], Token, Status, &Temperature);
	Token[0] = 43;
	for (unsigned Index = 17; Index < sizeof(Frame); ++Index)
		assert(!Parser.Feed(Frame[Index], Token, Status, &Temperature));
	assert(Temperature == -999);
	Frame[16] = 43;
	Frame[8] = 0xC4; Frame[9] = 9; Frame[10] = 0; Frame[11] = 0;
	for (auto Byte : Frame)
		Parser.Feed(Byte, Token, Status, &Temperature);
	assert(Status == ThermoStatus_Ok && Temperature == 25.0);
	std::cout << "Fresh response parser tests passed\n";
}
