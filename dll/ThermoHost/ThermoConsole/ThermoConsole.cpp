#include <iostream>
#include <Windows.h>

using GetApiVersionFunction = int(__cdecl*)(void);

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

	const int Version = pGetApiVersion();
	std::cout << "Explicitly loaded DLL API version: " << Version << '\n';

	FreeLibrary(Module);
	return 0;
}
