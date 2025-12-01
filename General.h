#pragma once
#include "IniLib/IniLib.h"

using namespace IniLib;

namespace General
{
	void LoadSettings(IniFile iniSettings);

	void ApplyPatches();
};

