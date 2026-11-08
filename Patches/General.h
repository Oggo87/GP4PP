#pragma once
#include "../IniLib/IniLib.h"

using namespace IniLib;

namespace General
{
	void LoadSettings(IniFile iniSettings);

	void ApplyPatches();

	// True when the Show Opponents Fuel Laps patch is applied, so every car's fuel laps are shown in a race
	bool IsOpponentsFuelLapsPatched();
};

