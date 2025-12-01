#include "GP4MemLib/GP4MemLib.h"
#include "IniLib/IniLib.h"
#include "Helpers.h"
#include "General.h"
#include "Assets.h"
#include "Pitcrew.h"
#include "CockpitVisor.h"
#include <map>
#include <string>
#include <windows.h>

using namespace std;
using namespace GP4MemLib;
using namespace IniLib;
using namespace GP4PP;


DWORD WINAPI MainThread(LPVOID param)
{

	//Get the DLL path and use it to load the ini file
	char currentPath[MAX_PATH];
	HMODULE dllHandle = GetModuleHandleA("GP4PP.dll");
	GetModuleFileNameA(dllHandle, currentPath, MAX_PATH);
	size_t pos = string(currentPath).find_last_of("\\/");
	string iniFilePath = string(currentPath).substr(0, pos) + "\\GP4PP.ini";

	IniFile iniSettings;

	// Load settings from ini file
	if (iniSettings.load(iniFilePath))
	{
		// Load Pitcrew Settings
		Pitcrew::LoadSettings(iniSettings);

		// Load General Settings
		General::LoadSettings(iniSettings);

		// Load Cockpit Visor Settings
		CockpitVisor::LoadSettings(iniSettings);

		// Load Asset Settings
		Assets::LoadSettings(iniSettings);

	}
	else
	{
		OutputGP4PPDebugString("Failed to open INI file");

		Assets::DefaultSettings();
	}

	//Apply General patches
	General::ApplyPatches();

	//Apply Asset patches
	Assets::ApplyPatches();

	//Apply Pitcrew patches
	Pitcrew::ApplyPatches();

	//Apply Cockpit Visor patches
	CockpitVisor::ApplyPatches();

	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {

	OutputGP4PPDebugString(MemUtils::dwordToString(ul_reason_for_call));

	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		CreateThread(NULL, 0, MainThread, NULL, 0, NULL);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}