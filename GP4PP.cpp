#include "Patches/General.h"
#include "Patches/CockpitVisor.h"
#include "Patches/HelmetVisor.h"
#include "Patches/Assets.h"
#include "Patches/Pitcrew.h"
#include "Patches/RearLight.h"
#include "Utils/Helpers.h"
#include "IniLib/IniLib.h"
#include <string>
#include <windows.h>

using namespace std;
using namespace IniLib;
using namespace GP4PP;


DWORD WINAPI MainThread(LPVOID param)
{

	//Get the DLL path and use it to load the ini file
	char currentPath[MAX_PATH];
	HMODULE dllHandle = GetModuleHandleA("GP4PP.dll");
	GetModuleFileNameA(dllHandle, currentPath, MAX_PATH);
	size_t pos = string(currentPath).find_last_of("\\/");
	string basePath = string(currentPath).substr(0, pos) + "\\";
	string iniFilePath = basePath + "GP4PP.ini";

	IniFile iniSettings;

	// Load settings from ini file
	if (iniSettings.load(iniFilePath))
	{
		// Load Pitcrew Settings
		Pitcrew::LoadSettings(iniSettings);

		// Load General Settings
		General::LoadSettings(iniSettings);

		// Load Cockpit Visor Settings
		CockpitVisor::LoadSettings(iniSettings, basePath);

		// Load Helmet Visor Settings
		HelmetVisor::LoadSettings(iniSettings, basePath);

		// Load Asset Settings
		Assets::LoadSettings(iniSettings);

		// Load Rear Light Settings
		//RearLight::LoadSettings(iniSettings);

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

	//Apply Helmet Visor patches
	HelmetVisor::ApplyPatches();

	//Apply Rear Light patches
	RearLight::ApplyPatches();

	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		OutputGP4PPDebugString("Attaching to process");
		CreateThread(NULL, 0, MainThread, NULL, 0, NULL);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		OutputGP4PPDebugString("Detaching from process");
		break;
	}
	return TRUE;
}