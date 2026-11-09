#pragma once
#include "../IniLib/IniLib.h"

using namespace IniLib;

namespace D3D
{
	void LoadSettings(IniFile iniSettings);

	void ApplyPatches();
};