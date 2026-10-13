#pragma once
#include "../IniLib/IniLib.h"

using namespace IniLib;

namespace SteeringWheels
{
	void LoadSettings(IniFile iniSettings);

	void ApplyPatches();
};
